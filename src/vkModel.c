#include <vulkan/vulkan.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <cglm/cglm.h>
#include <ktx.h>

#include <stdio.h>
#include <stdlib.h>

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
        aiProcess_MakeLeftHanded |
        aiProcess_FlipUVs |
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

    *pVertexSize += vertexSize * sizeof(VkModelVertex_t);
    *pIndexSize += indexSize * sizeof(u32);
    *pIndirectSize += node->mNumMeshes * sizeof(VkDrawIndexedIndirectCommand);

    for (u32 i = 0; i < node->mNumChildren; i++) {
        vkModelGetNodeSizes(scene, node->mChildren[i], pVertexSize, pIndexSize, pIndirectSize);
    }
}

void vkModelGetTexturesInfo(const struct aiScene* scene, const char* modelDirPath, u32* pImagesSize, u32* pImageCount, u32* pImageMipLevels, u32* pImageWidths, u32* pImageHeights) {
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
                if (pImageWidths != NULL && pImageHeights != NULL && pImageMipLevels != NULL) {
                    pImageMipLevels[curImageCount] = tex->numLevels;
                    pImageWidths[curImageCount] = tex->baseWidth;
                    pImageHeights[curImageCount] = tex->baseHeight;
                }

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
                if (pImageWidths != NULL && pImageHeights != NULL && pImageMipLevels != NULL) {
                    pImageMipLevels[curImageCount] = tex->numLevels;
                    pImageWidths[curImageCount] = tex->baseWidth;
                    pImageHeights[curImageCount] = tex->baseHeight;
                }

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
                if (pImageWidths != NULL && pImageHeights != NULL && pImageMipLevels != NULL) {
                    pImageMipLevels[curImageCount] = tex->numLevels;
                    pImageWidths[curImageCount] = tex->baseWidth;
                    pImageHeights[curImageCount] = tex->baseHeight;
                }

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

void vkModelGetSizes(const struct aiScene* scene, u32* pVertexSize, u32* pIndexSize, u32* pIndirectSize, u32* pStorageBufferMaterialsSize, u32* pStorageBufferMaterialIndicesSize) {
    *pVertexSize = 0;
    *pIndexSize = 0;
    *pIndirectSize = 0;
    *pStorageBufferMaterialsSize = scene->mNumMaterials * sizeof(VkModelMaterial_t);
    *pStorageBufferMaterialIndicesSize = scene->mNumMeshes * sizeof(u32);
    
    vkModelGetNodeSizes(scene, scene->mRootNode, pVertexSize, pIndexSize, pIndirectSize);
}

void vkModelProcessNode(const struct aiScene* scene, const struct aiNode* node, VkDeviceSize tempBufferVertexOffset, VkDeviceSize tempBufferIndexOffset, VkDeviceSize tempBufferIndirectOffset, VkDeviceSize tempBufferStorageOffset, void* pTempBufferRaw, u32* pCurVertexCount, u32* pCurIndexCount, u32* pCurIndirectCount, u32* pCurStorageOffset) {
    u32 curVertexCount = *pCurVertexCount;
    u32 curIndexCount = *pCurIndexCount;
    u32 curIndirectCount = *pCurIndirectCount;
    u32 curStorageOffset = *pCurStorageOffset;

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

        u32 materialIndex = mesh->mMaterialIndex;
        memcpy(pTempBufferRaw + tempBufferStorageOffset + curStorageOffset, &materialIndex, sizeof(u32));
        curStorageOffset += sizeof(u32);
    }

    *pCurVertexCount += curVertexCount - *pCurVertexCount;
    *pCurIndexCount += curIndexCount - *pCurIndexCount;
    *pCurIndirectCount += curIndirectCount - *pCurIndirectCount;
    *pCurStorageOffset += curStorageOffset - *pCurStorageOffset;

    for (u32 i = 0; i < node->mNumChildren; i++) {
        vkModelProcessNode(scene, node->mChildren[i], tempBufferVertexOffset, tempBufferIndexOffset, tempBufferIndirectOffset, tempBufferStorageOffset, pTempBufferRaw, pCurVertexCount, pCurIndexCount, pCurIndirectCount, pCurStorageOffset);
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

void vkModelCreate(const struct aiScene* scene, const char* modelDirPath, VkCommandBuffer tempCmdBuf, VkBuffer tempBuffer, VkDeviceSize tempBufferVertexOffset, VkDeviceSize tempBufferIndexOffset, VkDeviceSize tempBufferIndirectOffset, VkDeviceSize tempBufferStorageOffset, VkDeviceSize tempBufferTexturesOffset, VkDeviceSize storageBufferMaterialIndicesOffset, void* pTempBufferRaw, VkModel_t* pModel) {
    u32 curVertexCount = 0;
    u32 curIndexCount = 0;
    u32 curIndirectCount = 0;
    u32 curStorageOffset = 0;
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

        memcpy(pTempBufferRaw + tempBufferStorageOffset + curStorageOffset, &modelMat, sizeof(VkModelMaterial_t));
        curStorageOffset += sizeof(VkModelMaterial_t);
    }

    vkModelProcessNode(scene, scene->mRootNode, tempBufferVertexOffset, tempBufferIndexOffset, tempBufferIndirectOffset, tempBufferStorageOffset, pTempBufferRaw, &curVertexCount, &curIndexCount, &curIndirectCount, &curStorageOffset);

    pModel->drawCount = curIndirectCount;
    pModel->textureCount = curImageCount;

    VkBufferCopy copyInfo[5] = {};
    copyInfo[0].srcOffset = tempBufferVertexOffset;
    copyInfo[0].dstOffset = 0;
    copyInfo[0].size = curVertexCount * sizeof(VkModelVertex_t);

    copyInfo[1].srcOffset = tempBufferIndexOffset;
    copyInfo[1].dstOffset = 0;
    copyInfo[1].size = curIndexCount * sizeof(u32);

    copyInfo[2].srcOffset = tempBufferIndirectOffset;
    copyInfo[2].dstOffset = 0;
    copyInfo[2].size = curIndirectCount * sizeof(VkDrawIndexedIndirectCommand);

    copyInfo[3].srcOffset = tempBufferStorageOffset;
    copyInfo[3].dstOffset = 0;
    copyInfo[3].size = scene->mNumMaterials * sizeof(VkModelMaterial_t);

    copyInfo[4].srcOffset = tempBufferStorageOffset + scene->mNumMaterials * sizeof(VkModelMaterial_t);
    copyInfo[4].dstOffset = storageBufferMaterialIndicesOffset;
    copyInfo[4].size = scene->mNumMeshes * sizeof(u32);

    vkCmdCopyBuffer(tempCmdBuf, tempBuffer, pModel->vertexBuffer, 1, &copyInfo[0]);
    vkCmdCopyBuffer(tempCmdBuf, tempBuffer, pModel->indexBuffer, 1, &copyInfo[1]);
    vkCmdCopyBuffer(tempCmdBuf, tempBuffer, pModel->indirectBuffer, 1, &copyInfo[2]);
    vkCmdCopyBuffer(tempCmdBuf, tempBuffer, pModel->storageBuffer, 2, copyInfo + 3);
}

void vkModelGetDescriptorWrites(VkModel_t* pModel, VkSampler sampler, u32* pDescriptorBufferCount, VkDescriptorBufferInfo* pDescriptorBuffers, u32* pDescriptorImageCount, VkDescriptorImageInfo* pDescriptorImages, u32* pDescriptorWriteCount, VkWriteDescriptorSet* pDescriptorWrites) {
    *pDescriptorBufferCount = 3;
    *pDescriptorImageCount = pModel->textureCount;
    *pDescriptorWriteCount = 3 + pModel->textureCount;
    if (pDescriptorBuffers == NULL || pDescriptorImages == NULL || pDescriptorWrites == NULL) return;

    pDescriptorBuffers[0].buffer = pModel->hostVisibleStorageBuffer;
    pDescriptorBuffers[0].offset = 0;
    pDescriptorBuffers[0].range = VK_WHOLE_SIZE;

    pDescriptorBuffers[1].buffer = pModel->storageBuffer;
    pDescriptorBuffers[1].offset = 0;
    pDescriptorBuffers[1].range = pModel->materialsSize;

    pDescriptorBuffers[2].buffer = pModel->storageBuffer;
    pDescriptorBuffers[2].offset = pModel->materialIndicesOffset;
    pDescriptorBuffers[2].range = VK_WHOLE_SIZE;

    for (u32 i = 0; i < pModel->textureCount; i++) {
        pDescriptorImages[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        pDescriptorImages[i].imageView = pModel->views[i];
        pDescriptorImages[i].sampler = sampler;
        
        pDescriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        pDescriptorWrites[i].pNext = VK_NULL_HANDLE;
        pDescriptorWrites[i].descriptorCount = 1;
        pDescriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pDescriptorWrites[i].dstBinding = 3;
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