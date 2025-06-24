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

struct aiScene* vkModelLoadScene(const char* path) {
    return aiImportFile(path,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_GenUVCoords |
        aiProcess_FindInvalidData |
        aiProcess_FindDegenerates |
        // vulkan is left-handed
        aiProcess_MakeLeftHanded |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_EmbedTextures |
        aiProcess_FlipWindingOrder |
        // transition to zeux/meshoptimizer?
        aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality
    );
}

void vkModelGetSizes(struct aiScene* scene, u32* pVertexSize, u32* pIndexSize) {
    u32 vertexSize = 0;
    u32 indexSize = 0;

    for (u32 i = 0; i < scene->mNumMeshes; i++) {
        struct aiMesh* mesh = scene->mMeshes[i];
        vertexSize += mesh->mNumVertices;
        for (u32 j = 0; j < mesh->mNumFaces; j++) {
            indexSize += mesh->mFaces[j].mNumIndices;
        }
    }

    *pVertexSize = vertexSize * sizeof(vk_model_vertex_t);
    *pIndexSize = indexSize * sizeof(u32);
}

void vkModelInit(VkCommandBuffer tempCmdBuf, VkBuffer tempBuffer, VkDeviceSize tempBufferVertexOffset, VkDeviceSize tempBufferIndexOffset, void* pTempBufferRaw, struct aiScene* scene, vk_model_t* pModel) {
    u32 curVertexSize = 0;
    u32 curIndexSize = 0;

    for (u32 i = 0; i < scene->mNumMeshes; i++) {
        struct aiMesh* mesh = scene->mMeshes[i];

        for (u32 j = 0; j < mesh->mNumVertices; j++) {
            vk_model_vertex_t vertex = {};

            struct aiVector3D position = mesh->mVertices[j];
            vertex.position[0] = position.x;
            vertex.position[1] = position.y;
            vertex.position[2] = position.z;

            memcpy(pTempBufferRaw + tempBufferVertexOffset + curVertexSize, &vertex, sizeof(vk_model_vertex_t));
            curVertexSize += sizeof(vk_model_vertex_t);
        }

        for (u32 j = 0; j < mesh->mNumFaces; j++) {
            struct aiFace face = mesh->mFaces[i];

            memcpy(pTempBufferRaw + tempBufferIndexOffset + curIndexSize, face.mIndices, face.mNumIndices * sizeof(u32));
            curIndexSize += face.mNumIndices * sizeof(u32);
        }
    }

    VkBufferCopy copyInfo[2] = {};
    copyInfo[0].srcOffset = tempBufferVertexOffset;
    copyInfo[0].dstOffset = 0;
    copyInfo[0].size = curVertexSize;

    copyInfo[1].srcOffset = tempBufferIndexOffset;
    copyInfo[1].dstOffset = 0;
    copyInfo[1].size = curIndexSize;

    vkCmdCopyBuffer(tempCmdBuf, tempBuffer, pModel->vertexBuffer, 1, &copyInfo[0]);
    vkCmdCopyBuffer(tempCmdBuf, tempBuffer, pModel->indexBuffer, 1, &copyInfo[1]);
}

void vkModelUnloadScene(struct aiScene* scene) {
    aiReleaseImport(scene);
}