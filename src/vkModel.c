#include <vulkan/vulkan.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <cglm/cglm.h>
#include <ktx.h>

#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

#include "numtypes.h"
#include "vkFunctions.h"
#include "vkModel.h"
#include "util.h"
#include "vk.h"


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
        aiProcess_FlipUVs |
        aiProcess_RemoveRedundantMaterials |
        // transition to zeux/meshoptimizer?
        aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality
    );
}

void vkModelGetNodeSizes(const struct aiScene* scene, const struct aiNode* node, u32* pVertexSize, u32* pIndexSize, u32* pIndirectSize, u32* pTransformsSize) {
    u32 vertexSize = 0;
    u32 indexSize = 0;

    for (u32 i = 0; i < node->mNumMeshes; i++) {
        struct aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        vertexSize += mesh->mNumVertices;
        for (u32 j = 0; j < mesh->mNumFaces; j++) {
            indexSize += mesh->mFaces[j].mNumIndices * sizeof(u32);
        }
    }

    *pVertexSize += vertexSize * sizeof(VkModelVertex_t);
    *pIndexSize += indexSize * sizeof(u32);
    *pIndirectSize += node->mNumMeshes * sizeof(VkDrawIndexedIndirectCommand);
    *pTransformsSize += node->mNumMeshes > 0 ? sizeof(mat4) : 0;

    for (u32 i = 0; i < node->mNumChildren; i++) {
        vkModelGetNodeSizes(scene, node->mChildren[i], pVertexSize, pIndexSize, pIndirectSize, pTransformsSize);
    }
}

void vkModelGetTexturesInfo(const struct aiScene* scene, const char* modelDirPath, u32* pImagesSize, u32* pImageCount, u32* pImageMipLevels, VkFormat* pImageFormats, u32* pImageWidths, u32* pImageHeights) {
    u32 curImagesSize = 0;
    u32 curImageCount = 0;

    for (u32 i = 0; i < scene->mNumMaterials; i++) {
        const struct aiMaterial* mat = scene->mMaterials[i];

        if (aiGetMaterialTextureCount(mat, aiTextureType_DIFFUSE) > 0) {
            struct aiString path;

            if (aiGetMaterialTexture(mat, aiTextureType_DIFFUSE, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
                char p[512] = {};
                strcat(p, modelDirPath);
                strcat(p, path.data);

                ktxTexture2* tex;
                ktxTexture2_CreateFromNamedFile(p, 0, &tex);
                ktxTexture2_TranscodeBasis(tex, vkglobals.textureFormatKtx, 0);
                if (pImageFormats != NULL) pImageFormats[curImageCount] = vkglobals.textureFormatSRGB;
                if (pImageMipLevels != NULL) pImageMipLevels[curImageCount] = tex->numLevels;
                if (pImageWidths != NULL) pImageWidths[curImageCount] = tex->baseWidth;
                if (pImageHeights != NULL) pImageHeights[curImageCount] = tex->baseHeight;

                u32 size = ktxTexture_GetImageSize((ktxTexture*)tex, 0);
                size += getAlignCooficient(size, 16);
                for (u32 i = 1; i < tex->numLevels; i++) {
                    size += ktxTexture_GetImageSize((ktxTexture*)tex, i);
                    size += getAlignCooficient(size, 16);
                }
                curImagesSize += size;
                curImageCount++;
                ktxTexture2_Destroy(tex);
            }
        }
        
        if (aiGetMaterialTextureCount(mat, aiTextureType_NORMALS) > 0) {
            struct aiString path;

            if (aiGetMaterialTexture(mat, aiTextureType_NORMALS, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
                char p[512] = {};
                strcat(p, modelDirPath);
                strcat(p, path.data);

                ktxTexture2* tex;
                ktxTexture2_CreateFromNamedFile(p, 0, &tex);
                ktxTexture2_TranscodeBasis(tex, vkglobals.textureFormatKtx, 0);
                if (pImageFormats != NULL) pImageFormats[curImageCount] = vkglobals.textureFormat;
                if (pImageMipLevels != NULL) pImageMipLevels[curImageCount] = tex->numLevels;
                if (pImageWidths != NULL) pImageWidths[curImageCount] = tex->baseWidth;
                if (pImageHeights != NULL) pImageHeights[curImageCount] = tex->baseHeight;

                u32 size = ktxTexture_GetImageSize((ktxTexture*)tex, 0);
                size += getAlignCooficient(size, 16);
                for (u32 i = 1; i < tex->numLevels; i++) {
                    size += ktxTexture_GetImageSize((ktxTexture*)tex, i);
                    size += getAlignCooficient(size, 16);
                }
                curImagesSize += size;
                curImageCount++;
                ktxTexture2_Destroy(tex);
            }
        }

        if (aiGetMaterialTextureCount(mat, aiTextureType_GLTF_METALLIC_ROUGHNESS) > 0) {
            struct aiString path;

            if (aiGetMaterialTexture(mat, aiTextureType_GLTF_METALLIC_ROUGHNESS, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
                char p[512] = {};
                strcat(p, modelDirPath);
                strcat(p, path.data);

                ktxTexture2* tex;
                ktxTexture2_CreateFromNamedFile(p, 0, &tex);
                ktxTexture2_TranscodeBasis(tex, vkglobals.textureFormatKtx, 0);
                if (pImageFormats != NULL) pImageFormats[curImageCount] = vkglobals.textureFormat;
                if (pImageMipLevels != NULL) pImageMipLevels[curImageCount] = tex->numLevels;
                if (pImageWidths != NULL) pImageWidths[curImageCount] = tex->baseWidth;
                if (pImageHeights != NULL) pImageHeights[curImageCount] = tex->baseHeight;

                u32 size = ktxTexture_GetImageSize((ktxTexture*)tex, 0);
                size += getAlignCooficient(size, 16);
                for (u32 i = 1; i < tex->numLevels; i++) {
                    size += ktxTexture_GetImageSize((ktxTexture*)tex, i);
                    size += getAlignCooficient(size, 16);
                }
                curImagesSize += size;
                curImageCount++;
                ktxTexture2_Destroy(tex);
            }
        }
    }

    *pImagesSize = curImagesSize;
    *pImageCount = curImageCount;
}

void vkModelGetSizes(const struct aiScene* scene, u32* pVertexSize, u32* pIndexSize, u32* pIndirectSize, u32* pMaterialsSize, u32* pTransformsSize, u32* pMeshIndicesSize) {
    *pVertexSize = 0;
    *pIndexSize = 0;
    *pIndirectSize = 0;
    *pTransformsSize = 0;
    *pMaterialsSize = scene->mNumMaterials * sizeof(VkModelMaterial_t);
    *pMeshIndicesSize = scene->mNumMeshes * sizeof(VkModelMeshIndices_t);
    
    vkModelGetNodeSizes(scene, scene->mRootNode, pVertexSize, pIndexSize, pIndirectSize, pTransformsSize);
}

void vkModelProcessNode(const struct aiScene* scene, const struct aiNode* node, u32* pNodeIndex, struct aiMatrix4x4* mat, VkDeviceSize tempBufferVertexOffset, VkDeviceSize tempBufferIndexOffset, VkDeviceSize tempBufferIndirectOffset, VkDeviceSize tempBufferTransformsOffset, VkDeviceSize tempBufferMeshIndicesOffset, void* pTempBufferRaw, u32* pCurVertexCount, u32* pCurIndexCount, u32* pCurIndirectCount, u32* pCurTransformOffset, u32* pCurMeshIndexOffset) {
    u32 curVertexCount = *pCurVertexCount;
    u32 curIndexCount = *pCurIndexCount;
    u32 curIndirectCount = *pCurIndirectCount;
    u32 curMeshIndexOffset = *pCurMeshIndexOffset;

    for (u32 i = 0; i < node->mNumMeshes; i++) {
        struct aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

        VkDrawIndexedIndirectCommand drawCommand = {};
        drawCommand.firstIndex = curIndexCount;
        drawCommand.firstInstance = 0;
        drawCommand.instanceCount = 1;
        drawCommand.vertexOffset = curVertexCount;

        for (u32 j = 0; j < mesh->mNumVertices; j++) {
            VkModelVertex_t vertex = {};

            struct aiVector3D position = mesh->mVertices[j];
            vertex.position[0] = position.x;
            vertex.position[1] = position.y;
            vertex.position[2] = position.z;

            struct aiVector3D normal = mesh->mNormals[j];
            vertex.normal[0] = normal.x;
            vertex.normal[1] = normal.y;
            vertex.normal[2] = normal.z;

            struct aiVector3D tangent = mesh->mTangents[j];
            vertex.tangent[0] = tangent.x;
            vertex.tangent[1] = tangent.y;
            vertex.tangent[2] = tangent.z;

            struct aiVector3D uv = mesh->mTextureCoords[0][j];
            vertex.uv[0] = uv.x;
            vertex.uv[1] = uv.y;

            memcpy(pTempBufferRaw + tempBufferVertexOffset + curVertexCount * sizeof(VkModelVertex_t), &vertex, sizeof(VkModelVertex_t));
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

        VkModelMeshIndices_t indicies = {};
        indicies.materialIndex = mesh->mMaterialIndex;
        indicies.nodeIndex = *pNodeIndex;
        memcpy(pTempBufferRaw + tempBufferMeshIndicesOffset + curMeshIndexOffset, &indicies, sizeof(VkModelMeshIndices_t));
        curMeshIndexOffset += sizeof(VkModelMeshIndices_t);
    }

    aiMultiplyMatrix4(mat, &node->mTransformation);

    if (node->mNumMeshes > 0) {
        struct aiMatrix4x4 copy = *mat;
        aiTransposeMatrix4(&copy);

        memcpy(pTempBufferRaw + tempBufferTransformsOffset + *pCurTransformOffset, &copy, sizeof(mat4));
        *pCurTransformOffset += sizeof(mat4);
        
        *pNodeIndex += 1;
    }

    *pCurVertexCount += curVertexCount - *pCurVertexCount;
    *pCurIndexCount += curIndexCount - *pCurIndexCount;
    *pCurIndirectCount += curIndirectCount - *pCurIndirectCount;
    *pCurMeshIndexOffset += curMeshIndexOffset - *pCurMeshIndexOffset;

    for (u32 i = 0; i < node->mNumChildren; i++) {
        struct aiMatrix4x4 next = *mat;

        vkModelProcessNode(scene, node->mChildren[i], pNodeIndex, &next, tempBufferVertexOffset, tempBufferIndexOffset, tempBufferIndirectOffset, tempBufferTransformsOffset, tempBufferMeshIndicesOffset, pTempBufferRaw, pCurVertexCount, pCurIndexCount, pCurIndirectCount, pCurTransformOffset, pCurMeshIndexOffset);
    }
}

u32 vkModelLoadTexture(const struct aiMaterial* mat, enum aiTextureType type, const char* modelDirPath, VkCommandBuffer tempCmdBuf, VkBuffer tempBuffer, VkDeviceSize tempBufferTexturesOffset, void* pTempBufferRaw, u32* pCurImageOffset, u32* pCurImageCount, VkModel_t* pModel) {
    struct aiString path;

    if (aiGetMaterialTexture(mat, type, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL) != AI_SUCCESS) return 1;

    if (path.data[0] == '*') {
        printf("models with embedded textures are not supporte\n");
        exit(1);
        /*
        TODO: implement embedded textures

        u32 id = atoi(path->data + 1);

        const struct aiTexture* tex = scene->mTextures[id];
        */
    } else {
        char p[512] = {};
        strcat(p, modelDirPath);
        strcat(p, path.data);

        ktxTexture2* tex;
        ktxTexture2_CreateFromNamedFile(p, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &tex);
        ktxTexture2_TranscodeBasis(tex, vkglobals.textureFormatKtx, 0);

        u32 curImageOffset = 0;
        VkDeviceSize mipOffsets[tex->numLevels];

        for (u32 i = 0; i < tex->numLevels; i++) {
            u32 size = ktxTexture_GetImageSize((ktxTexture*)tex, i);
            u64 offset = 0;
            ktxTexture_GetImageOffset((ktxTexture*)tex, i, 0, 0, &offset);
            memcpy(pTempBufferRaw + tempBufferTexturesOffset + *pCurImageOffset + curImageOffset, ktxTexture_GetData((ktxTexture*)tex) + offset, size);

            mipOffsets[i] = tempBufferTexturesOffset + *pCurImageOffset + curImageOffset;
            curImageOffset += size + getAlignCooficient(size, 16);
        }

        copyTempBufferToImage(tempCmdBuf, tempBuffer, mipOffsets, pModel->textures[*pCurImageCount], tex->baseWidth, tex->baseHeight, 1, 0, tex->numLevels, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        ktxTexture2_Destroy(tex);
        *pCurImageOffset += curImageOffset; // BC and ASTC require offset alignment to their block size (we're using only 16 block size)
        (*pCurImageCount)++;
    }

    return 0;
}

void vkModelCreate(const struct aiScene* scene, const char* modelDirPath, VkCommandBuffer tempCmdBuf, VkBuffer tempBuffer, VkDeviceSize tempBufferVertexOffset, VkDeviceSize tempBufferIndexOffset, VkDeviceSize tempBufferIndirectOffset, VkDeviceSize tempBufferMaterialsOffset, VkDeviceSize tempBufferTransformsOffset, VkDeviceSize tempBufferMeshIndicesOffset, VkDeviceSize tempBufferTexturesOffset, VkDeviceSize transformsOffset, VkDeviceSize meshIndicesOffset, void* pTempBufferRaw, VkModel_t* pModel) {
    u32 curVertexCount = 0;
    u32 curIndexCount = 0;
    u32 curIndirectCount = 0;
    u32 curTransformOffset = 0;
    u32 curMaterialOffset = 0;
    u32 curMeshIndexOffset = 0;
    u32 curImageOffset = 0;
    u32 curImageCount = 0;

    for (u32 i = 0; i < scene->mNumMaterials; i++) {
        const struct aiMaterial* mat = scene->mMaterials[i];

        VkModelMaterial_t modelMat = {};
        u32 alignCooficient = getAlignCooficient(tempBufferTexturesOffset, 16);

        if (aiGetMaterialTextureCount(mat, aiTextureType_DIFFUSE) > 0) {
            modelMat.diffuseIndex = curImageCount;
            if (vkModelLoadTexture(mat, aiTextureType_DIFFUSE, modelDirPath, tempCmdBuf, tempBuffer, tempBufferTexturesOffset + alignCooficient, pTempBufferRaw, &curImageOffset, &curImageCount, pModel) != 0)
                modelMat.diffuseIndex = -1;
        } else {
            modelMat.diffuseIndex = -1;
        }
        
        if (aiGetMaterialTextureCount(mat, aiTextureType_NORMALS) > 0) {
            modelMat.normalMapIndex = curImageCount;
            if (vkModelLoadTexture(mat, aiTextureType_NORMALS, modelDirPath, tempCmdBuf, tempBuffer, tempBufferTexturesOffset + alignCooficient, pTempBufferRaw, &curImageOffset, &curImageCount, pModel) != 0)
                modelMat.normalMapIndex = -1;
        } else {
            modelMat.normalMapIndex = -1;
        }

        if (aiGetMaterialTextureCount(mat, aiTextureType_GLTF_METALLIC_ROUGHNESS) > 0) {
            modelMat.metallicRoughnessIndex = curImageCount;
            if (vkModelLoadTexture(mat, aiTextureType_GLTF_METALLIC_ROUGHNESS, modelDirPath, tempCmdBuf, tempBuffer, tempBufferTexturesOffset + alignCooficient, pTempBufferRaw, &curImageOffset, &curImageCount, pModel) != 0)
                modelMat.metallicRoughnessIndex = -1;
        } else {
            modelMat.metallicRoughnessIndex = -1;
        }

        memcpy(pTempBufferRaw + tempBufferMaterialsOffset + curMaterialOffset, &modelMat, sizeof(VkModelMaterial_t));
        curMaterialOffset += sizeof(VkModelMaterial_t);
    }

    u32 nodeIndex = 0;

    struct aiMatrix4x4 mat = {};
    aiIdentityMatrix4(&mat);

    vkModelProcessNode(scene, scene->mRootNode, &nodeIndex, &mat, tempBufferVertexOffset, tempBufferIndexOffset, tempBufferIndirectOffset, tempBufferTransformsOffset, tempBufferMeshIndicesOffset, pTempBufferRaw, &curVertexCount, &curIndexCount, &curIndirectCount, &curTransformOffset, &curMeshIndexOffset);

    pModel->drawCount = curIndirectCount;
    pModel->textureCount = curImageCount;

    VkBufferCopy copyInfo[6] = {};
    copyInfo[0].srcOffset = tempBufferVertexOffset;
    copyInfo[0].dstOffset = 0;
    copyInfo[0].size = curVertexCount * sizeof(VkModelVertex_t);

    copyInfo[1].srcOffset = tempBufferIndexOffset;
    copyInfo[1].dstOffset = 0;
    copyInfo[1].size = curIndexCount * sizeof(u32);

    copyInfo[2].srcOffset = tempBufferIndirectOffset;
    copyInfo[2].dstOffset = 0;
    copyInfo[2].size = curIndirectCount * sizeof(VkDrawIndexedIndirectCommand);

    copyInfo[3].srcOffset = tempBufferMaterialsOffset;
    copyInfo[3].dstOffset = 0;
    copyInfo[3].size = scene->mNumMaterials * sizeof(VkModelMaterial_t);

    copyInfo[4].srcOffset = tempBufferTransformsOffset;
    copyInfo[4].dstOffset = transformsOffset;
    copyInfo[4].size = curTransformOffset;

    copyInfo[5].srcOffset = tempBufferMeshIndicesOffset;
    copyInfo[5].dstOffset = meshIndicesOffset;
    copyInfo[5].size = scene->mNumMeshes * sizeof(VkModelMeshIndices_t);

    vkCmdCopyBuffer(tempCmdBuf, tempBuffer, pModel->vertexBuffer, 1, &copyInfo[0]);
    vkCmdCopyBuffer(tempCmdBuf, tempBuffer, pModel->indexBuffer, 1, &copyInfo[1]);
    vkCmdCopyBuffer(tempCmdBuf, tempBuffer, pModel->indirectBuffer, 1, &copyInfo[2]);
    vkCmdCopyBuffer(tempCmdBuf, tempBuffer, pModel->storageBuffer, 3, copyInfo + 3);
}

void vkModelGetDescriptorWrites(VkModel_t* pModel, VkSampler sampler, u32* pDescriptorBufferCount, VkDescriptorBufferInfo* pDescriptorBuffers, u32* pDescriptorImageCount, VkDescriptorImageInfo* pDescriptorImages, u32* pDescriptorWriteCount, VkWriteDescriptorSet* pDescriptorWrites) {
    *pDescriptorBufferCount = 4;
    *pDescriptorImageCount = pModel->textureCount;
    *pDescriptorWriteCount = 4 + pModel->textureCount;
    if (pDescriptorBuffers == NULL || pDescriptorImages == NULL || pDescriptorWrites == NULL) return;

    pDescriptorBuffers[0].buffer = pModel->hostVisibleStorageBuffer;
    pDescriptorBuffers[0].offset = 0;
    pDescriptorBuffers[0].range = VK_WHOLE_SIZE;

    pDescriptorBuffers[1].buffer = pModel->storageBuffer;
    pDescriptorBuffers[1].offset = 0;
    pDescriptorBuffers[1].range = pModel->materialsSize;

    pDescriptorBuffers[2].buffer = pModel->storageBuffer;
    pDescriptorBuffers[2].offset = pModel->transformsOffset;
    pDescriptorBuffers[2].range = pModel->transformsSize;

    pDescriptorBuffers[3].buffer = pModel->storageBuffer;
    pDescriptorBuffers[3].offset = pModel->meshIndicesOffset;
    pDescriptorBuffers[3].range = VK_WHOLE_SIZE;

    for (u32 i = 0; i < pModel->textureCount; i++) {
        pDescriptorImages[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        pDescriptorImages[i].imageView = pModel->views[i];
        pDescriptorImages[i].sampler = sampler;
        
        pDescriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        pDescriptorWrites[i].pNext = VK_NULL_HANDLE;
        pDescriptorWrites[i].descriptorCount = 1;
        pDescriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pDescriptorWrites[i].dstBinding = 4;
        pDescriptorWrites[i].dstArrayElement = i;
        pDescriptorWrites[i].dstSet = pModel->descriptorSet;
        pDescriptorWrites[i].pImageInfo = pDescriptorImages + i;
    }

    pDescriptorWrites[pModel->textureCount].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    pDescriptorWrites[pModel->textureCount].pNext = VK_NULL_HANDLE;
    pDescriptorWrites[pModel->textureCount].descriptorCount = 1;
    pDescriptorWrites[pModel->textureCount].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pDescriptorWrites[pModel->textureCount].dstBinding = 0;
    pDescriptorWrites[pModel->textureCount].dstArrayElement = 0;
    pDescriptorWrites[pModel->textureCount].dstSet = pModel->descriptorSet;
    pDescriptorWrites[pModel->textureCount].pBufferInfo = &pDescriptorBuffers[0];

    pDescriptorWrites[pModel->textureCount + 1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    pDescriptorWrites[pModel->textureCount + 1].pNext = VK_NULL_HANDLE;
    pDescriptorWrites[pModel->textureCount + 1].descriptorCount = 1;
    pDescriptorWrites[pModel->textureCount + 1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pDescriptorWrites[pModel->textureCount + 1].dstBinding = 1;
    pDescriptorWrites[pModel->textureCount + 1].dstArrayElement = 0;
    pDescriptorWrites[pModel->textureCount + 1].dstSet = pModel->descriptorSet;
    pDescriptorWrites[pModel->textureCount + 1].pBufferInfo = &pDescriptorBuffers[1];

    pDescriptorWrites[pModel->textureCount + 2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    pDescriptorWrites[pModel->textureCount + 2].pNext = VK_NULL_HANDLE;
    pDescriptorWrites[pModel->textureCount + 2].descriptorCount = 1;
    pDescriptorWrites[pModel->textureCount + 2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pDescriptorWrites[pModel->textureCount + 2].dstBinding = 2;
    pDescriptorWrites[pModel->textureCount + 2].dstArrayElement = 0;
    pDescriptorWrites[pModel->textureCount + 2].dstSet = pModel->descriptorSet;
    pDescriptorWrites[pModel->textureCount + 2].pBufferInfo = &pDescriptorBuffers[2];

    pDescriptorWrites[pModel->textureCount + 3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    pDescriptorWrites[pModel->textureCount + 3].pNext = VK_NULL_HANDLE;
    pDescriptorWrites[pModel->textureCount + 3].descriptorCount = 1;
    pDescriptorWrites[pModel->textureCount + 3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pDescriptorWrites[pModel->textureCount + 3].dstBinding = 3;
    pDescriptorWrites[pModel->textureCount + 3].dstArrayElement = 0;
    pDescriptorWrites[pModel->textureCount + 3].dstSet = pModel->descriptorSet;
    pDescriptorWrites[pModel->textureCount + 3].pBufferInfo = &pDescriptorBuffers[3];
}

void vkModelDestroy(VkModel_t* pModel) {
    for (u32 i = 0; i < pModel->textureCount; i++ ) {
        vkDestroyImage(vkglobals.device, pModel->textures[i], VK_NULL_HANDLE);
        vkDestroyImageView(vkglobals.device, pModel->views[i], VK_NULL_HANDLE);
    }
    free(pModel->textures);

    vkDestroyBuffer(vkglobals.device, pModel->hostVisibleStorageBuffer, VK_NULL_HANDLE);
    vkDestroyBuffer(vkglobals.device, pModel->storageBuffer, VK_NULL_HANDLE);
    vkDestroyBuffer(vkglobals.device, pModel->vertexBuffer, VK_NULL_HANDLE);
    vkDestroyBuffer(vkglobals.device, pModel->indexBuffer, VK_NULL_HANDLE);
    vkDestroyBuffer(vkglobals.device, pModel->indirectBuffer, VK_NULL_HANDLE);
}

void vkModelUnloadScene(const struct aiScene* scene) {
    aiReleaseImport(scene);
}