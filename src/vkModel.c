#include <vulkan/vulkan.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <cglm/cglm.h>

#include "numtypes.h"
#include "vkFunctions.h"
#include "vkModel.h"

struct aiLogStream logStream;

void vkModelInitLogStream() {
    logStream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT, NULL);
}

void vkModelAttachLogStream() {
    aiAttachLogStream(&logStream);
}

void vkModelDetachLogStream() {
    aiDetachLogStream(&logStream);
}

const struct aiScene* vkModelLoadScene(const char* path) {
    return aiImportFile(path,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_GenUVCoords |
        aiProcess_MakeLeftHanded |
        aiProcess_RemoveRedundantMaterials |
        // transition to zeux/meshoptimizer?
        aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality
    );
}

void vkModelGetNodeSizes(const struct aiScene* scene, const struct aiNode* node, u32* pVertexSize, u32* pIndexSize, u32* pIndirectSize) {
    u32 vertexSize = 0;
    u32 indexSize = 0;

    for (u32 i = 0; i < node->mNumMeshes; i++) {
        struct aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        vertexSize += mesh->mNumVertices;
        for (u32 j = 0; j < mesh->mNumFaces; j++) {
            indexSize += mesh->mFaces[j].mNumIndices;
        }
    }

    *pVertexSize += vertexSize * sizeof(vk_model_vertex_t);
    *pIndexSize += indexSize * sizeof(u32);
    *pIndirectSize += node->mNumMeshes * sizeof(VkDrawIndexedIndirectCommand);

    for (u32 i = 0; i < node->mNumChildren; i++) {
        vkModelGetNodeSizes(scene, node->mChildren[i], pVertexSize, pIndexSize, pIndirectSize);
    }
}

void vkModelGetSizes(const struct aiScene* scene, u32* pVertexSize, u32* pIndexSize, u32* pIndirectSize) {
    *pVertexSize = 0;
    *pIndexSize = 0;
    *pIndirectSize = 0;
    
    vkModelGetNodeSizes(scene, scene->mRootNode, pVertexSize, pIndexSize, pIndirectSize);
}

void vkModelProcessNode(const struct aiScene* scene, const struct aiNode* node, VkDeviceSize tempBufferVertexOffset, VkDeviceSize tempBufferIndexOffset, VkDeviceSize tempBufferIndirectOffset, void* pTempBufferRaw, u32* pCurVertexCount, u32* pCurIndexCount, u32* pCurIndirectCount) {
    u32 curVertexCount = *pCurVertexCount;
    u32 curIndexCount = *pCurIndexCount;
    u32 curIndirectCount = *pCurIndirectCount;

    for (u32 i = 0; i < node->mNumMeshes; i++) {
        struct aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

        VkDrawIndexedIndirectCommand drawCommand = {};
        drawCommand.firstIndex = curIndexCount;
        drawCommand.firstInstance = 0;
        drawCommand.instanceCount = 1;
        drawCommand.vertexOffset = curVertexCount;

        for (u32 j = 0; j < mesh->mNumVertices; j++) {
            vk_model_vertex_t vertex = {};

            struct aiVector3D position = mesh->mVertices[j];
            vertex.position[0] = position.x;
            vertex.position[1] = position.y * -1;
            vertex.position[2] = position.z;

            struct aiVector3D normal = mesh->mNormals[j];
            vertex.normal[0] = normal.x;
            vertex.normal[1] = normal.y * -1;
            vertex.normal[2] = normal.z;

            struct aiVector3D tangent = mesh->mTangents[j];
            vertex.tangent[0] = tangent.x;
            vertex.tangent[1] = tangent.y * -1;
            vertex.tangent[2] = tangent.z;

            struct aiVector3D uv = mesh->mTextureCoords[0][j];
            vertex.uv[0] = uv.x;
            vertex.uv[1] = uv.y;

            memcpy(pTempBufferRaw + tempBufferVertexOffset + curVertexCount * sizeof(vk_model_vertex_t), &vertex, sizeof(vk_model_vertex_t));
            curVertexCount++;
        }

        for (u32 j = 0; j < mesh->mNumFaces; j++) {
            struct aiFace face = mesh->mFaces[j];

            memcpy(pTempBufferRaw + tempBufferIndexOffset + curIndexCount * sizeof(u32), face.mIndices, face.mNumIndices * sizeof(u32));
            curIndexCount += face.mNumIndices;
        }

        drawCommand.indexCount = curIndexCount - drawCommand.firstIndex;
        memcpy(pTempBufferRaw + tempBufferIndirectOffset + curIndirectCount * sizeof(VkDrawIndexedIndirectCommand), &drawCommand, sizeof(VkDrawIndexedIndirectCommand));
        curIndirectCount++;
    }

    *pCurVertexCount += curVertexCount - *pCurVertexCount;
    *pCurIndexCount += curIndexCount - *pCurIndexCount;
    *pCurIndirectCount += curIndirectCount - *pCurIndirectCount;

    for (u32 i = 0; i < node->mNumChildren; i++) {
        vkModelProcessNode(scene, node->mChildren[i], tempBufferVertexOffset, tempBufferIndexOffset, tempBufferIndirectOffset, pTempBufferRaw, pCurVertexCount, pCurIndexCount, pCurIndirectCount);
    }
}

void vkModelInit(const struct aiScene* scene, VkCommandBuffer tempCmdBuf, VkBuffer tempBuffer, VkDeviceSize tempBufferVertexOffset, VkDeviceSize tempBufferIndexOffset, VkDeviceSize tempBufferIndirectOffset, void* pTempBufferRaw, vk_model_t* pModel) {
    u32 curVertexCount = 0;
    u32 curIndexCount = 0;
    u32 curIndirectCount = 0;

    vkModelProcessNode(scene, scene->mRootNode, tempBufferVertexOffset, tempBufferIndexOffset, tempBufferIndirectOffset, pTempBufferRaw, &curVertexCount, &curIndexCount, &curIndirectCount);

    pModel->drawCount = curIndirectCount;

    VkBufferCopy copyInfo[3] = {};
    copyInfo[0].srcOffset = tempBufferVertexOffset;
    copyInfo[0].dstOffset = 0;
    copyInfo[0].size = curVertexCount * sizeof(vk_model_vertex_t);

    copyInfo[1].srcOffset = tempBufferIndexOffset;
    copyInfo[1].dstOffset = 0;
    copyInfo[1].size = curIndexCount * sizeof(u32);

    copyInfo[2].srcOffset = tempBufferIndirectOffset;
    copyInfo[2].dstOffset = 0;
    copyInfo[2].size = curIndirectCount * sizeof(VkDrawIndexedIndirectCommand);

    vkCmdCopyBuffer(tempCmdBuf, tempBuffer, pModel->vertexBuffer, 1, &copyInfo[0]);
    vkCmdCopyBuffer(tempCmdBuf, tempBuffer, pModel->indexBuffer, 1, &copyInfo[1]);
    vkCmdCopyBuffer(tempCmdBuf, tempBuffer, pModel->indirectBuffer, 1, &copyInfo[2]);
}

void vkModelUnloadScene(const struct aiScene* scene) {
    aiReleaseImport(scene);
}