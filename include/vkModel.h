#ifndef VK_MODEL_H
#define VK_MODEL_H

#include <vulkan/vulkan.h>
#include <assimp/scene.h>
#include <cglm/cglm.h>

typedef struct {
    vec3 position;
    vec3 normal;
    vec3 tangent;
    vec2 uv;
} vk_model_vertex_t;

typedef struct {
    i32 diffuseIndex;
    i32 normalMapIndex;
} vk_model_material_t;

typedef struct {
    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;
    VkBuffer indirectBuffer;
    u32 drawCount;

    VkBuffer storageBuffer;
    VkDeviceSize storageBufferMaterialsSize;
    VkDeviceSize storageBufferMaterialIndicesOffset;

    VkImage* textures;
    VkImageView* views;
    u32 textureCount;

    VkDescriptorSet descriptorSet;
} vk_model_t;


void vkModelInitLogStream();
void vkModelAttachLogStream();
void vkModelDetachLogStream();
const struct aiScene* vkModelLoadScene(const char* path);
void vkModelGetTexturesInfo(const struct aiScene* scene, u32* pImagesSize, u32* pImageCount, u32* pImageWidths, u32* pImageHeights);
void vkModelGetSizes(const struct aiScene* scene, u32* pVertexSize, u32* pIndexSize, u32* pIndirectSize, u32* pStorageBufferMaterialsSize, u32* pStorageBufferMaterialIndicesSize);
void vkModelCreate(const struct aiScene* scene, VkCommandBuffer tempCmdBuf, VkBuffer tempBuffer, VkDeviceSize tempBufferVertexOffset, VkDeviceSize tempBufferIndexOffset, VkDeviceSize tempBufferIndirectOffset, VkDeviceSize tempBufferStorageOffset, VkDeviceSize tempBufferTexturesOffset, VkDeviceSize storageBufferMaterialIndicesOffset, void* pTempBufferRaw, vk_model_t* pModel);
void vkModelGetDescriptorWrites(vk_model_t* pModel, u32* pDescriptorBufferCount, VkDescriptorBufferInfo* pDescriptorBuffers, u32* pDescriptorImageCount, VkDescriptorImageInfo* pDescriptorImages, u32* pDescriptorWriteCount, VkWriteDescriptorSet* pDescriptorWrites);
void vkModelDestroy(vk_model_t* pModel);
void vkModelUnloadScene(const struct aiScene* scene);

#endif