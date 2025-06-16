#ifndef UTIL_H
#define UTIL_H

#include <vulkan/vulkan.h>

#include "numtypes.h"

void copyTempBufferToImage(VkCommandBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize bufferOffset, VkImage image, u32 w, u32 h, u32 arrayLayers, u32 baseArrayLayer, VkImageLayout newLayout);
void allocateMemory(VkDeviceMemory* pMem, VkDeviceSize size, u32 memoryTypeIndex);
void createBuffer(VkBuffer* pBuffer, VkBufferUsageFlags usage, VkDeviceSize size);
void createImage(VkImage* pImage, i32 w, i32 h, VkFormat textureFormat, u32 arrayLayers, VkImageUsageFlags usage);
void createImageView(VkImageView* pView, VkImage image, VkImageViewType type, VkFormat textureFormat, u32 arrayLayers, u32 baseArrayLayer, VkImageAspectFlags aspect);
u32 getMemoryTypeIndex(u32 filter, VkMemoryPropertyFlags props);
VkShaderModule createShaderModuleFromAsset(char* path);
VkPipelineCache loadPipelineCache(char* path);
void storePipelineCache(VkPipelineCache cache, char* path);

// alignment stuff
VkDeviceSize getAlignCooficient(VkDeviceSize size, u32 alignment);
VkDeviceSize getAlignCooficientByTwo(VkDeviceSize size, u32 alignment1, u32 alignment2);

typedef struct {
    VkDeviceSize offset;
    VkDeviceSize size;
} offset_size_t;

offset_size_t getAlignedMaxOffsetAndMinSize(VkDeviceSize unalignedOffset, VkDeviceSize size, u32 alignment);

#endif