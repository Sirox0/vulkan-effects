#ifndef VK_INIT_H
#define VK_INIT_H

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <ktx.h>

#include <stdio.h>
#include <stdlib.h>

#include "numtypes.h"

#define VK_ASSERT(expression, message) \
    { \
        VkResult err = expression; \
        if (err != VK_SUCCESS) { \
            printf(__FILE__ ":%d VkResult: %d. " message, __LINE__, err); \
            exit(1); \
        } \
    }

typedef struct {
    SDL_Window* window;
    u8 loopActive;
    u32 deltaTime;
    u32 time;
    f32 fps;

    VkFilter textureFilter;
    VkFormat textureFormat;
    VkFormat textureFormatSRGB;
    ktx_transcode_fmt_e textureFormatKtx;

    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceProperties2KHR deviceProperties;
    VkPhysicalDeviceFeatures2KHR deviceFeatures;
    VkPhysicalDeviceMemoryProperties2KHR deviceMemoryProperties;
    VkDevice device;
    u32 queueFamilyIndex;
    VkQueue queue;

    VkCommandPool commandPool;
    VkCommandPool shortCommandPool;

    VkSurfaceKHR surface;
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VkPresentModeKHR surfacePresentMode;
    VkSurfaceFormatKHR surfaceFormat;
    VkSwapchainKHR swapchain;
    VkExtent2D swapchainExtent;
} VulkanGlobals_t;

u32 getMemoryTypeIndex(u32 filter, VkMemoryPropertyFlags props);
VkShaderModule createShaderModuleFromFile(char* path);

void vkInit();
void vkQuit();

extern VulkanGlobals_t vkglobals;

#endif