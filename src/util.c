#include <vulkan/vulkan.h>

#include <stdio.h>
#include <stdlib.h>

#include "numtypes.h"
#include "mathext.h"
#include "vkFunctions.h"
#include "vk.h"
#include "util.h"

void copyTempBufferToImage(VkCommandBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize bufferOffset, VkImage image, u32 w, u32 h, u32 arrayLayers, u32 baseArrayLayer, VkImageLayout newLayout) {
    VkImageMemoryBarrier imageBarriers[2] = {};
    imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageBarriers[0].image = image;
    imageBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBarriers[0].subresourceRange.baseArrayLayer = baseArrayLayer;
    imageBarriers[0].subresourceRange.layerCount = arrayLayers;
    imageBarriers[0].subresourceRange.baseMipLevel = 0;
    imageBarriers[0].subresourceRange.levelCount = 1;
    imageBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarriers[0].srcAccessMask = 0;
    imageBarriers[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    imageBarriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageBarriers[1].image = image;
    imageBarriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBarriers[1].subresourceRange.baseArrayLayer = baseArrayLayer;
    imageBarriers[1].subresourceRange.layerCount = arrayLayers;
    imageBarriers[1].subresourceRange.baseMipLevel = 0;
    imageBarriers[1].subresourceRange.levelCount = 1;
    imageBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarriers[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageBarriers[1].dstAccessMask = 0;
    imageBarriers[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageBarriers[1].newLayout = newLayout;

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &imageBarriers[0]);

    VkBufferImageCopy copyInfo = {};
    copyInfo.bufferOffset = bufferOffset;
    copyInfo.imageExtent = (VkExtent3D){w, h, 1};
    copyInfo.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyInfo.imageSubresource.baseArrayLayer = baseArrayLayer;
    copyInfo.imageSubresource.layerCount = arrayLayers;
    copyInfo.imageSubresource.mipLevel = 0;

    vkCmdCopyBufferToImage(cmdBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyInfo);

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &imageBarriers[1]);
}

void createBuffer(VkBuffer* pBuffer, VkBufferUsageFlags usage, VkDeviceSize size) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.usage = usage;
    bufferInfo.size = size;

    VK_ASSERT(vkCreateBuffer(vkglobals.device, &bufferInfo, VK_NULL_HANDLE, pBuffer), "failed to create buffer\n");
}

void vkAllocateMemoryCluster(VkMemoryAllocClusterInfo_t* info, VkDeviceMemory* pMem) {
    VkMemoryRequirements memReqs[info->handleCount];
    VkDeviceSize offsets[info->handleCount];
    u32 memoryTypeFilter;

    for (u32 i = 0; i < info->handleCount; i++) {
        switch (info->handleType) {
            case VK_MEMORY_ALLOC_CLUSTER_HANDLE_TYPE_BUFFER:
                vkGetBufferMemoryRequirements(vkglobals.device, ((const VkBuffer*)info->pHandles)[i], &memReqs[i]);
                break;
            case VK_MEMORY_ALLOC_CLUSTER_HANDLE_TYPE_IMAGE:
                vkGetImageMemoryRequirements(vkglobals.device, ((const VkImage*)info->pHandles)[i], &memReqs[i]);
                break;
        }

        if (i == 0) {
            offsets[i] = 0;
            memoryTypeFilter = memReqs[i].memoryTypeBits;
        } else {
            offsets[i] = offsets[i-1] + memReqs[i-1].size + getAlignCooficient(offsets[i-1] + memReqs[i-1].size, memReqs[i].alignment);
            memoryTypeFilter &= memReqs[i].memoryTypeBits;
        }
    }

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = offsets[info->handleCount - 1] + memReqs[info->handleCount - 1].size;
    allocInfo.memoryTypeIndex = getMemoryTypeIndex(memoryTypeFilter, info->memoryProperties);

    VK_ASSERT(vkAllocateMemory(vkglobals.device, &allocInfo, VK_NULL_HANDLE, pMem), "failed to allocate memory\n");

    for (u32 i = 0; i < info->handleCount; i++) {
        switch (info->handleType) {
            case VK_MEMORY_ALLOC_CLUSTER_HANDLE_TYPE_BUFFER:
                VK_ASSERT(vkBindBufferMemory(vkglobals.device, ((const VkBuffer*)info->pHandles)[i], *pMem, offsets[i]), "failed to bind buffer memory\n");
                break;
            case VK_MEMORY_ALLOC_CLUSTER_HANDLE_TYPE_IMAGE:
                VK_ASSERT(vkBindImageMemory(vkglobals.device, ((const VkImage*)info->pHandles)[i], *pMem,  offsets[i]), "failed to bind image memory\n");
                break;
        }
    }
}

void createImage(VkImage* pImage, i32 w, i32 h, VkFormat textureFormat, u32 arrayLayers, VkImageUsageFlags usage, VkImageCreateFlags flags) {
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags = flags;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.extent = (VkExtent3D){w, h, 1};
    imageInfo.format = textureFormat;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = arrayLayers;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VK_ASSERT(vkCreateImage(vkglobals.device, &imageInfo, VK_NULL_HANDLE, pImage), "failed to create image\n");
}

void createImageView(VkImageView* pView, VkImage image, VkImageViewType type, VkFormat textureFormat, u32 arrayLayers, u32 baseArrayLayer, VkImageAspectFlags aspect) {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.viewType = type;
    viewInfo.image = image;
    viewInfo.format = textureFormat;
    viewInfo.components = (VkComponentMapping){VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    viewInfo.subresourceRange.aspectMask = aspect;
    viewInfo.subresourceRange.baseArrayLayer = baseArrayLayer;
    viewInfo.subresourceRange.layerCount = arrayLayers;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;

    VK_ASSERT(vkCreateImageView(vkglobals.device, &viewInfo, VK_NULL_HANDLE, pView), "failed to create image view\n");
}

u32 getMemoryTypeIndex(u32 filter, VkMemoryPropertyFlags props) {
    for (u32 i = 0; i < vkglobals.deviceMemoryProperties.memoryTypeCount; i++) {
        if (filter & (1 << i) && ((vkglobals.deviceMemoryProperties.memoryTypes[i].propertyFlags & props) == props)) {
            return i;
        }
    }
    printf("failed to find required memory type\n");
    exit(1);
}

VkShaderModule createShaderModuleFromAsset(char* path) {
    FILE* f = fopen(path, "rb");
    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    u8 shaderCode[fsize];
    fread(shaderCode, fsize, 1, f);
    fclose(f);

    VkShaderModule module;
    VkShaderModuleCreateInfo moduleInfo = {};
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.codeSize = fsize;
    moduleInfo.pCode = (u32*)shaderCode;

    VK_ASSERT(vkCreateShaderModule(vkglobals.device, &moduleInfo, VK_NULL_HANDLE, &module), "failed to create shader module\n");
    return module;
}

VkPipelineCache loadPipelineCache(char* path) {
    FILE* f = fopen(path, "rb");
    size_t fsize = 0;
    u8* cacheData = NULL;

    if (f != NULL) {
        fseek(f, 0, SEEK_END);
        fsize = ftell(f);
        fseek(f, 0, SEEK_SET);

        cacheData = (u8*)malloc(sizeof(u8) * fsize);
        fread(cacheData, fsize, 1, f);
        fclose(f);
    }

    VkPipelineCache cache;
    VkPipelineCacheCreateInfo cacheInfo = {};
    cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    cacheInfo.initialDataSize = fsize;
    cacheInfo.pInitialData = cacheData;

    VK_ASSERT(vkCreatePipelineCache(vkglobals.device, &cacheInfo, VK_NULL_HANDLE, &cache), "failed to create pipeline cache\n");
    if (cacheData != NULL) free(cacheData);
    return cache;
}

void storePipelineCache(VkPipelineCache cache, char* path) {
    size_t cacheSize;
    vkGetPipelineCacheData(vkglobals.device, cache, &cacheSize, VK_NULL_HANDLE);
    u8* cacheData = (u8*)malloc(sizeof(u8) * cacheSize);
    vkGetPipelineCacheData(vkglobals.device, cache, &cacheSize, cacheData);

    FILE* f = fopen(path, "wb");

    if (f != NULL) {
        fwrite(cacheData, cacheSize, 1, f);
        fclose(f);
    }
    free(cacheData);
}

VkDeviceSize getAlignCooficient(VkDeviceSize size, u32 alignment) {
    return alignment - (size % alignment);
}

VkDeviceSize getAlignCooficientByTwo(VkDeviceSize size, u32 alignment1, u32 alignment2) {
    u32 alignFactor = lcm(alignment1, alignment2);
    return alignFactor - (size % alignFactor);
}