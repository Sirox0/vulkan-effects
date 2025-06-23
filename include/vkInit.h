#ifndef VK_INIT_H
#define VK_INIT_H

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>

#include "numtypes.h"

#define VK_ASSERT(expression, message) \
    if (expression != VK_SUCCESS) { \
        printf(message); \
        exit(1); \
    }

typedef struct {
    SDL_Window* window;

    VkFilter textureFilter;

    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
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
    u32 swapchainImageCount;
    VkImage* swapchainImages;
    VkImageView* swapchainImageViews;
    VkCommandBuffer cmdBuffer;
} vulkan_globals_t;

u32 getMemoryTypeIndex(u32 filter, VkMemoryPropertyFlags props);
VkShaderModule createShaderModuleFromFile(char* path);

void vkInit();
void vkQuit();

extern vulkan_globals_t vkglobals;

#endif