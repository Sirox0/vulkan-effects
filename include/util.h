#ifndef UTIL_H
#define UTIL_H

#include <vulkan/vulkan.h>

#include "numtypes.h"

typedef enum {
    VK_MEMORY_ALLOC_CLUSTER_HANDLE_TYPE_BUFFER = 0,
    VK_MEMORY_ALLOC_CLUSTER_HANDLE_TYPE_IMAGE = 1
} VkMemoryAllocClusterHandleType_t;

typedef struct {
    VkMemoryPropertyFlags memoryProperties;
    VkMemoryAllocClusterHandleType_t handleType;
    u32 handleCount;
    const void* pHandles;
} VkMemoryAllocClusterInfo_t;

void copyTempBufferToImage(VkCommandBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize* bufferOffsets, VkImage image, u32 w, u32 h, u32 arrayLayers, u32 baseArrayLayer, u32 mipLevels, u32 baseMipLevel, VkImageLayout newLayout);
void vkAllocateMemoryCluster(VkMemoryAllocClusterInfo_t* info, VkDeviceMemory* pMem);
void createBuffer(VkBuffer* pBuffer, VkBufferUsageFlags usage, VkDeviceSize size);
void createImage(VkImage* pImage, i32 w, i32 h, VkFormat textureFormat, u32 arrayLayers, u32 mipLevels, VkImageUsageFlags usage, VkImageCreateFlags flags);
void createImageView(VkImageView* pView, VkImage image, VkImageViewType type, VkFormat textureFormat, u32 arrayLayers, u32 baseArrayLayer, u32 mipLevels, u32 baseMipLevel, VkImageAspectFlags aspect);
u32 getMemoryTypeIndex(u32 filter, VkMemoryPropertyFlags props);
VkShaderModule createShaderModuleFromAsset(char* path);
VkPipelineCache loadPipelineCache(char* path);
void storePipelineCache(VkPipelineCache cache, char* path);

// alignment stuff
VkDeviceSize getAlignCooficient(VkDeviceSize size, u32 alignment);
VkDeviceSize getAlignCooficientByTwo(VkDeviceSize size, u32 alignment1, u32 alignment2);

#endif