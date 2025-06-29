#include <vulkan/vulkan.h>

#include "numtypes.h"
#include "vk.h"
#include "vkFunctions.h"
#include "util.h"

void tempResourcesCreate(u32 cmdBufferCount, VkCommandBuffer* cmdBuffers, u32 fenceCount, VkFence* fences) {
    VkCommandBufferAllocateInfo garbageCmdBuffersInfo = {};
    garbageCmdBuffersInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    garbageCmdBuffersInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    garbageCmdBuffersInfo.commandPool = vkglobals.shortCommandPool;
    garbageCmdBuffersInfo.commandBufferCount = cmdBufferCount;

    VK_ASSERT(vkAllocateCommandBuffers(vkglobals.device, &garbageCmdBuffersInfo, cmdBuffers), "failed to allocate command buffers\n");

    VkCommandBufferBeginInfo garbageCmdBuffersBeginInfo = {};
    garbageCmdBuffersBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    garbageCmdBuffersBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    for (u32 i = 0; i < cmdBufferCount; i++) VK_ASSERT(vkBeginCommandBuffer(cmdBuffers[i], &garbageCmdBuffersBeginInfo), "failed to begin command buffer\n");

    VkFenceCreateInfo garbageFenceInfo = {};
    garbageFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    for (u32 i = 0; i < fenceCount; i++) {
        VK_ASSERT(vkCreateFence(vkglobals.device, &garbageFenceInfo, VK_NULL_HANDLE, &fences[i]), "failed to create fence\n");
    }
}

void tempResourcesWaitAndDestroy(u32 cmdBufferCount, VkCommandBuffer* cmdBuffers, u32 bufferCount, VkBuffer* buffers, u32 bufferMemCount, VkDeviceMemory* buffersMem, u32 fenceCount, VkFence* fences) {
    if (fenceCount > 0) VK_ASSERT(vkWaitForFences(vkglobals.device, fenceCount, fences, VK_TRUE, 0xFFFFFFFFFFFFFFFF), "failed to wait for fences\n");
    for (u32 i = 0; i < fenceCount; i++) {
        vkDestroyFence(vkglobals.device, fences[i], VK_NULL_HANDLE);
    }
    vkFreeCommandBuffers(vkglobals.device, vkglobals.shortCommandPool, cmdBufferCount, cmdBuffers);
    for (u32 i = 0; i < bufferCount; i++) {
        vkDestroyBuffer(vkglobals.device, buffers[i], VK_NULL_HANDLE);
    }
    for (u32 i = 0; i < bufferMemCount; i++) {
        vkFreeMemory(vkglobals.device, buffersMem[i], VK_NULL_HANDLE);
    }
}