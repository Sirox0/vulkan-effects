#ifndef VK_MODEL_H
#define VK_MODEL_H

#include <vulkan/vulkan.h>
#include <assimp/scene.h>
#include <cglm/cglm.h>
#include <vulkan/vulkan_core.h>

#include "numtypes.h"

typedef struct {
    vec3 position;
    vec3 normal;
    vec3 tangent;
    vec2 uv;
} VkModelVertex_t;

typedef struct {
    i32 diffuseIndex;
    i32 normalMapIndex;
    i32 metallicRoughnessIndex;
} VkModelMaterial_t;

typedef struct {
    i32 materialIndex;
    u32 nodeIndex;
} VkModelMeshIndices_t;

typedef struct {
    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;
    VkBuffer indirectBuffer;
    u32 drawCount;

    VkBuffer storageBuffer;
    VkDeviceSize materialsSize;
    VkDeviceSize transformsSize;
    VkDeviceSize transformsOffset;
    VkDeviceSize meshIndicesOffset;

    VkBuffer hostVisibleStorageBuffer;

    VkImage* textures;
    VkImageView* views;
    u32 textureCount;

    VkDescriptorSet descriptorSet;
} VkModel_t;


void vkModelInitLogStream();
void vkModelAttachLogStream();
void vkModelDetachLogStream();
const struct aiScene* vkModelLoadScene(const char* path);
void vkModelGetTexturesInfo(const struct aiScene* scene, const char* modelDirPath, u32* pImagesSize, u32* pImageCount, u32* pImageMipLevels, u32* pImageWidths, u32* pImageHeights);
void vkModelGetSizes(const struct aiScene* scene, u32* pVertexSize, u32* pIndexSize, u32* pIndirectSize, u32* pMaterialsSize, u32* pTransformsSize, u32* pMeshIndicesSize);
void vkModelCreate(const struct aiScene* scene, const char* modelDirPath, VkCommandBuffer tempCmdBuf, VkBuffer tempBuffer, VkDeviceSize tempBufferVertexOffset, VkDeviceSize tempBufferIndexOffset, VkDeviceSize tempBufferIndirectOffset, VkDeviceSize tempBufferMaterialsOffset, VkDeviceSize tempBufferTransformsOffset, VkDeviceSize tempBufferMeshIndicesOffset, VkDeviceSize tempBufferTexturesOffset, VkDeviceSize transformsOffset, VkDeviceSize meshIndicesOffset, void* pTempBufferRaw, VkModel_t* pModel);
void vkModelGetDescriptorWrites(VkModel_t* pModel, VkSampler sampler, u32* pDescriptorBufferCount, VkDescriptorBufferInfo* pDescriptorBuffers, u32* pDescriptorImageCount, VkDescriptorImageInfo* pDescriptorImages, u32* pDescriptorWriteCount, VkWriteDescriptorSet* pDescriptorWrites);
void vkModelDestroy(VkModel_t* pModel);
void vkModelUnloadScene(const struct aiScene* scene);

#endif