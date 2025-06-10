#include <vulkan/vulkan.h>

#include <stdio.h>
#include <stdlib.h>

#include "numtypes.h"
#include "mathext.h"
#include "vkFunctions.h"
#include "vkInit.h"
#include "game.h"
#include "util.h"

void copyBufferToImage(VkCommandBuffer cmdBuffer, VkBuffer buffer, VkImage image, u32 w, u32 h, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkImageMemoryBarrier imageBarrier = {};
    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageBarrier.image = image;
    imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBarrier.subresourceRange.baseArrayLayer = 0;
    imageBarrier.subresourceRange.layerCount = 1;
    imageBarrier.subresourceRange.baseMipLevel = 0;
    imageBarrier.subresourceRange.levelCount = 1;
    imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.srcAccessMask = 0;
    imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageBarrier.oldLayout = oldLayout;
    imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &imageBarrier);

    VkBufferImageCopy copyInfo = {};
    copyInfo.imageExtent = (VkExtent3D){w, h, 1};
    copyInfo.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyInfo.imageSubresource.baseArrayLayer = 0;
    copyInfo.imageSubresource.layerCount = 1;
    copyInfo.imageSubresource.mipLevel = 0;

    vkCmdCopyBufferToImage(cmdBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyInfo);

    imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageBarrier.dstAccessMask = 0;
    imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageBarrier.newLayout = newLayout;

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &imageBarrier);
}

void createBuffer(VkBuffer* pBuffer, VkBufferUsageFlags usage, VkDeviceSize size) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.usage = usage;
    bufferInfo.size = size;

    VK_ASSERT(vkCreateBuffer(vkglobals.device, &bufferInfo, VK_NULL_HANDLE, pBuffer), "failed to create buffer\n");
}

void allocateMemory(VkDeviceMemory* pMem, VkDeviceSize size, u32 memoryTypeIndex) {
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    VK_ASSERT(vkAllocateMemory(vkglobals.device, &allocInfo, VK_NULL_HANDLE, pMem), "failed to allocate memory\n");
}

void createImage(VkImage* pImage, i32 w, i32 h, VkFormat textureFormat, VkImageUsageFlags usage) {
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.extent = (VkExtent3D){w, h, 1};
    imageInfo.format = textureFormat;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VK_ASSERT(vkCreateImage(vkglobals.device, &imageInfo, VK_NULL_HANDLE, pImage), "failed to create image\n");
}

void createImageView(VkImageView* pView, VkImage image, VkFormat textureFormat, VkImageAspectFlags aspect) {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.image = image;
    viewInfo.format = textureFormat;
    viewInfo.components = (VkComponentMapping){VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    viewInfo.subresourceRange.aspectMask = aspect;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;

    VK_ASSERT(vkCreateImageView(vkglobals.device, &viewInfo, VK_NULL_HANDLE, pView), "failed to create image view\n");
}

void createSampler(VkSampler* pSampler) {
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

    VK_ASSERT(vkCreateSampler(vkglobals.device, &samplerInfo, VK_NULL_HANDLE, pSampler), "failed to create sampler\n");
}

VkDeviceSize getAlignCooficient(VkDeviceSize size, u32 alignment) {
    return alignment - (size % alignment);
}

VkDeviceSize getAlignCooficientByTwo(VkDeviceSize size, u32 alignment1, u32 alignment2) {
    u32 alignFactor = lcm(alignment1, alignment2);
    return alignFactor - (size % alignFactor);
}

offset_size_t getAlignedOffsetAndSize(VkDeviceSize unalignedOffset, VkDeviceSize size, u32 alignment) {
    VkDeviceSize maxOffset = 0, newsize = 0;
    do {
        maxOffset += alignment;
    } while (maxOffset < unalignedOffset);
    maxOffset -= alignment;
    do {
        newsize += alignment;
    } while (newsize < size);
    return (offset_size_t){maxOffset, newsize};
}