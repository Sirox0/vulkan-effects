#include <vulkan/vulkan.h>
#include <SDL3/SDL_vulkan.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include <vulkan/vulkan_core.h>

#include "numtypes.h"
#include "vkFunctions.h"
#include "vk.h"
#include "mathext.h"
#include "config.h"

VulkanGlobals_t vkglobals = {};

void vkInit() {
    loadVulkanLoaderFunctions();

    #define DEVICE_EXTENSION_COUNT 10
    const char* deviceExtensions[DEVICE_EXTENSION_COUNT] = {"VK_KHR_swapchain", "VK_KHR_maintenance1", "VK_KHR_dynamic_rendering", "VK_KHR_depth_stencil_resolve", "VK_KHR_create_renderpass2", "VK_KHR_multiview", "VK_KHR_maintenance2", "VK_KHR_shader_draw_parameters", "VK_EXT_descriptor_indexing", "VK_KHR_maintenance3"};
    #define DEVICE_OPTIONAL_EXTENSION_COUNT 2
    const char* deviceOptionalExtensions[DEVICE_OPTIONAL_EXTENSION_COUNT] = {"VK_IMG_filter_cubic", "VK_EXT_filter_cubic"};
    u8 optionalExts[DEVICE_OPTIONAL_EXTENSION_COUNT] = {0};
    #define INSTANCE_EXTENSION_COUNT 1
    const char* instanceExtensions[INSTANCE_EXTENSION_COUNT] = {"VK_KHR_get_physical_device_properties2"};

    const char* vkLayers[] = {"VK_LAYER_KHRONOS_validation"};

    {
        u32 sdlInstanceExtensionCount = 0;
        const char** sdlInstanceExtensions = (const char**)SDL_Vulkan_GetInstanceExtensions(&sdlInstanceExtensionCount);

        const char* finalInstanceExtensions[sdlInstanceExtensionCount + INSTANCE_EXTENSION_COUNT];
        for (u32 i = 0; i < sdlInstanceExtensionCount; i++) finalInstanceExtensions[i] = sdlInstanceExtensions[i];
        for (u32 i = sdlInstanceExtensionCount; i < sdlInstanceExtensionCount + INSTANCE_EXTENSION_COUNT; i++) finalInstanceExtensions[i] = instanceExtensions[i - sdlInstanceExtensionCount];

        VkInstanceCreateInfo instanceInfo = {};
        instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceInfo.enabledLayerCount = config.validation;
        instanceInfo.ppEnabledLayerNames = vkLayers;
        instanceInfo.enabledExtensionCount = sdlInstanceExtensionCount + INSTANCE_EXTENSION_COUNT;
        instanceInfo.ppEnabledExtensionNames = (const char**)finalInstanceExtensions;

        VK_ASSERT(vkCreateInstance(&instanceInfo, VK_NULL_HANDLE, &vkglobals.instance), "failed to create vulkan instance\n");
    }

    loadVulkanInstanceFunctions(vkglobals.instance);

    if (SDL_Vulkan_CreateSurface(vkglobals.window, vkglobals.instance, VK_NULL_HANDLE, &vkglobals.surface) != true) {
        printf("failed to create vulkan surface for sdl window\n");
        exit(1);
    }

    {
        u32 physicalDeviceCount;
        vkEnumeratePhysicalDevices(vkglobals.instance, &physicalDeviceCount, VK_NULL_HANDLE);
        VkPhysicalDevice physicalDevices[physicalDeviceCount];
        vkEnumeratePhysicalDevices(vkglobals.instance, &physicalDeviceCount, physicalDevices);

        u8 foundDevice = 0;
        for (u32 i = 0; i < physicalDeviceCount; i++) {
            u32 extensionPropertyCount;
            vkEnumerateDeviceExtensionProperties(physicalDevices[i], VK_NULL_HANDLE, &extensionPropertyCount, VK_NULL_HANDLE);
            VkExtensionProperties extensionProperties[extensionPropertyCount];
            vkEnumerateDeviceExtensionProperties(physicalDevices[i], VK_NULL_HANDLE, &extensionPropertyCount, extensionProperties);

            u8 foundExts[DEVICE_EXTENSION_COUNT] = {0};
            u8 foundOptionalExts[DEVICE_OPTIONAL_EXTENSION_COUNT] = {0}; 
            for (u32 availableExt = 0; availableExt < extensionPropertyCount; availableExt++) {
                for (u32 requiredExt = 0; requiredExt < DEVICE_EXTENSION_COUNT; requiredExt++) {
                    if (strcmp(deviceExtensions[requiredExt], extensionProperties[availableExt].extensionName) == 0) {
                        foundExts[requiredExt] = 1;
                        break;
                    }
                }

                for (u32 optionalExt = 0; optionalExt < DEVICE_OPTIONAL_EXTENSION_COUNT; optionalExt++) {
                    if (strcmp(deviceOptionalExtensions[optionalExt], extensionProperties[availableExt].extensionName) == 0) {
                        foundOptionalExts[optionalExt] = 1;
                        break;
                    }
                }
            }
            u8 foundExtsSum = 0;
            for (u32 requiredExt = 0; requiredExt < DEVICE_EXTENSION_COUNT; requiredExt++) {
                foundExtsSum += foundExts[requiredExt];
            }
            if (foundExtsSum < DEVICE_EXTENSION_COUNT) continue;

            u32 queueFamilyPropertyCount;
            vkGetPhysicalDeviceQueueFamilyProperties2KHR(physicalDevices[i], &queueFamilyPropertyCount, VK_NULL_HANDLE);

            VkQueueFamilyProperties2KHR queueFamilyProperties[queueFamilyPropertyCount];
            for (u32 i = 0; i < queueFamilyPropertyCount; i++) {
                queueFamilyProperties[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2_KHR;
                queueFamilyProperties[i].pNext = VK_NULL_HANDLE;
            }

            vkGetPhysicalDeviceQueueFamilyProperties2KHR(physicalDevices[i], &queueFamilyPropertyCount, queueFamilyProperties);

            u8 foundQueueFamily = 0;
            for (u32 queueFamilyIndex = 0; queueFamilyIndex < queueFamilyPropertyCount; queueFamilyIndex++) {
                if (queueFamilyProperties[queueFamilyIndex].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT && queueFamilyProperties[queueFamilyIndex].queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) {
                    VkBool32 canPresent;
                    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[i], queueFamilyIndex, vkglobals.surface, &canPresent);

                    if (canPresent == VK_TRUE) {
                        vkglobals.queueFamilyIndex = queueFamilyIndex;
                        foundQueueFamily = 1;
                        break;
                    }
                }
            }
            if (!foundQueueFamily) continue;

            u32 surfacePresentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevices[i], vkglobals.surface, &surfacePresentModeCount, VK_NULL_HANDLE);
            VkPresentModeKHR surfacePresentModes[surfacePresentModeCount];
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevices[i], vkglobals.surface, &surfacePresentModeCount, surfacePresentModes);

            if (config.vsync) {
                if (config.vsyncRelaxed) {
                    u8 fifoRelaxedPresentModeSupported = 0;
                    for (u32 i = 0; i < surfacePresentModeCount; i++) {
                        if (surfacePresentModes[i] == VK_PRESENT_MODE_FIFO_RELAXED_KHR) {
                            fifoRelaxedPresentModeSupported = 1;
                            vkglobals.surfacePresentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
                        }
                    }
                    if (!fifoRelaxedPresentModeSupported) vkglobals.surfacePresentMode = VK_PRESENT_MODE_FIFO_KHR;
                } else vkglobals.surfacePresentMode = VK_PRESENT_MODE_FIFO_KHR;
            } else {
                u8 immediatePresentModeSupported = 0;
                for (u32 i = 0; i < surfacePresentModeCount; i++) {
                    if (surfacePresentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                        immediatePresentModeSupported = 1;
                        vkglobals.surfacePresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                    }
                }
                if (!immediatePresentModeSupported) {
                    // mailbox is kinda similar to immediate, its just that it discards the frames presentation engine dont need
                    u8 mailboxPresentModeSupported = 0;
                    if (surfacePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
                        mailboxPresentModeSupported = 1;
                        vkglobals.surfacePresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                    }
                    if (!mailboxPresentModeSupported) vkglobals.surfacePresentMode = VK_PRESENT_MODE_FIFO_KHR;
                }
            }

            u32 surfaceFormatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevices[i], vkglobals.surface, &surfaceFormatCount, VK_NULL_HANDLE);
            if (surfaceFormatCount == 0) continue;
            VkSurfaceFormatKHR surfaceFormats[surfaceFormatCount];
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevices[i], vkglobals.surface, &surfaceFormatCount, surfaceFormats);

            u8 foundUNORMformat = 0;
            for (u32 i = 0; i < surfaceFormatCount; i++) {
                switch (surfaceFormats[i].format) {
                    case VK_FORMAT_R8G8B8A8_UNORM:
                    case VK_FORMAT_B8G8R8A8_UNORM:
                    case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
                    case VK_FORMAT_R8G8B8_UNORM:
                    case VK_FORMAT_B8G8R8_UNORM:
                    case VK_FORMAT_R8G8_UNORM:
                    case VK_FORMAT_R8_UNORM:
                        foundUNORMformat = 1;
                        vkglobals.surfaceFormat = surfaceFormats[i];
                    default: break;
                }
                if (foundUNORMformat) break;
            }
            if (!foundUNORMformat) vkglobals.surfaceFormat = surfaceFormats[0];

            vkglobals.deviceMemoryProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2_KHR;
            vkglobals.deviceMemoryProperties.pNext = VK_NULL_HANDLE;
            vkglobals.deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
            vkglobals.deviceProperties.pNext = VK_NULL_HANDLE;
            vkglobals.deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
            vkglobals.deviceFeatures.pNext = VK_NULL_HANDLE;

            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevices[i], vkglobals.surface, &vkglobals.surfaceCapabilities);
            vkGetPhysicalDeviceMemoryProperties2KHR(physicalDevices[i], &vkglobals.deviceMemoryProperties);
            vkGetPhysicalDeviceProperties2KHR(physicalDevices[i], &vkglobals.deviceProperties);
            vkGetPhysicalDeviceFeatures2KHR(physicalDevices[i], &vkglobals.deviceFeatures);

            vkglobals.physicalDevice = physicalDevices[i];
            foundDevice = 1;
            memcpy(optionalExts, foundOptionalExts, sizeof(optionalExts));
            break;
        }

        if (!foundDevice) {
            printf("failed to find a suitable vulkan device, try updating your gpu drivers\n");
            exit(1);
        }
    }

    {
        VkFormatProperties2KHR properties;
        properties.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2_KHR;
        properties.pNext = VK_NULL_HANDLE;

        vkglobals.textureFormat = VK_FORMAT_UNDEFINED;

        if (vkglobals.deviceFeatures.features.textureCompressionASTC_LDR) {
            vkGetPhysicalDeviceFormatProperties2KHR(vkglobals.physicalDevice, VK_FORMAT_ASTC_4x4_UNORM_BLOCK, &properties);

            if (properties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT && properties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR) {
                vkglobals.textureFormat = VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
                vkglobals.textureFormatSRGB = VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
                vkglobals.textureFormatKtx = KTX_TTF_ASTC_4x4_RGBA;
                goto foundFormat;
            }
        }

        if (vkglobals.deviceFeatures.features.textureCompressionBC) {
            vkGetPhysicalDeviceFormatProperties2KHR(vkglobals.physicalDevice, VK_FORMAT_BC7_UNORM_BLOCK, &properties);

            if (properties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT && properties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR) {
                vkglobals.textureFormat = VK_FORMAT_BC7_UNORM_BLOCK;
                vkglobals.textureFormatSRGB = VK_FORMAT_BC7_SRGB_BLOCK;
                vkglobals.textureFormatKtx = KTX_TTF_BC7_RGBA;
                goto foundFormat;
            }
        }

        if (vkglobals.textureFormat == VK_FORMAT_UNDEFINED) {
            vkGetPhysicalDeviceFormatProperties2KHR(vkglobals.physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &properties);

            vkglobals.textureFormat = VK_FORMAT_R8G8B8A8_UNORM;
            vkglobals.textureFormatSRGB = VK_FORMAT_R8G8B8A8_SRGB;
            vkglobals.textureFormatKtx = KTX_TTF_RGBA32;
        }

        foundFormat:

        // if cubic filtering is available check its support for texture format
        if (optionalExts[0] || optionalExts[1]) {
            // IMG and EXT alias each other here
            if (properties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_EXT && config.preferredTextureFilter == 2) {
                vkglobals.textureFilter = VK_FILTER_CUBIC_EXT;
            } else if (properties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT && config.preferredTextureFilter >= 1) {
                vkglobals.textureFilter = VK_FILTER_LINEAR;
            } else {
                vkglobals.textureFilter = VK_FILTER_NEAREST;
            }
        } else {
            if (properties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT && config.preferredTextureFilter >= 1) {
                vkglobals.textureFilter = VK_FILTER_LINEAR;
            } else {
                vkglobals.textureFilter = VK_FILTER_NEAREST;
            }
        }
        
    }

    {
        f32 priorities[] = {1.0f};

        VkPhysicalDeviceDynamicRenderingFeaturesKHR deviceDynamicRenderingFeatures = {};
        deviceDynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
        deviceDynamicRenderingFeatures.dynamicRendering = VK_TRUE;

        VkPhysicalDeviceDescriptorIndexingFeaturesEXT deviceDescriptorIndexingFeatures = {};
        deviceDescriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
        deviceDescriptorIndexingFeatures.pNext = &deviceDynamicRenderingFeatures;
        deviceDescriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
        deviceDescriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;

        VkPhysicalDeviceFeatures2KHR deviceFeatures = {};
        deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
        deviceFeatures.pNext = &deviceDescriptorIndexingFeatures;
        deviceFeatures.features.multiDrawIndirect = VK_TRUE;
        if (config.maxAnisotropy) deviceFeatures.features.samplerAnisotropy = VK_TRUE;
        if (config.wireframe) deviceFeatures.features.fillModeNonSolid = VK_TRUE;

        VkDeviceQueueCreateInfo deviceQueueInfo = {};
        deviceQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceQueueInfo.pQueuePriorities = priorities;
        deviceQueueInfo.queueCount = 1;
        deviceQueueInfo.queueFamilyIndex = vkglobals.queueFamilyIndex;

        const char* finalDeviceExtensions[DEVICE_EXTENSION_COUNT + DEVICE_OPTIONAL_EXTENSION_COUNT];
        u32 deviceExtensionIdx = 0;
        for (u32 i = 0; i < DEVICE_EXTENSION_COUNT; i++) {
            finalDeviceExtensions[deviceExtensionIdx] = deviceExtensions[i];
            deviceExtensionIdx++;
        }
        for (u32 i = 0; i < DEVICE_OPTIONAL_EXTENSION_COUNT; i++) {
            if (optionalExts[i]) {
                finalDeviceExtensions[deviceExtensionIdx] = deviceOptionalExtensions[i];
                deviceExtensionIdx++;
            }
        }

        VkDeviceCreateInfo deviceInfo = {};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.enabledLayerCount = config.validation;
        deviceInfo.ppEnabledLayerNames = vkLayers;
        deviceInfo.enabledExtensionCount = deviceExtensionIdx;
        deviceInfo.ppEnabledExtensionNames = finalDeviceExtensions;
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pQueueCreateInfos = &deviceQueueInfo;
        deviceInfo.pNext = &deviceFeatures;

        VK_ASSERT(vkCreateDevice(vkglobals.physicalDevice, &deviceInfo, VK_NULL_HANDLE, &vkglobals.device), "failed to create vulkan logical device\n");
    }

    loadVulkanDeviceFunctions(vkglobals.device);

    vkGetDeviceQueue(vkglobals.device, vkglobals.queueFamilyIndex, 0, &vkglobals.queue);

    {
        VkCommandPoolCreateInfo commandPoolInfo = {};
        commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolInfo.queueFamilyIndex = vkglobals.queueFamilyIndex;

        VK_ASSERT(vkCreateCommandPool(vkglobals.device, &commandPoolInfo, VK_NULL_HANDLE, &vkglobals.commandPool), "failed to create command pool\n");

        commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        VK_ASSERT(vkCreateCommandPool(vkglobals.device, &commandPoolInfo, VK_NULL_HANDLE, &vkglobals.shortCommandPool), "failed to create command pool\n");
    }

    // 0xFFFFFFFF means that the extent is defined by the swapchain
    if (vkglobals.surfaceCapabilities.currentExtent.width != 0xFFFFFFFF) {
        vkglobals.swapchainExtent = vkglobals.surfaceCapabilities.currentExtent;
    } else {
        i32 w;
        i32 h;
        if (SDL_GetWindowSizeInPixels(vkglobals.window, &w, &h) != true) {
            printf("failed to get window size from sdl\n");
            exit(1);
        }

        vkglobals.swapchainExtent.width = w;
        vkglobals.swapchainExtent.height = h;
        clamp(&vkglobals.swapchainExtent.width, vkglobals.surfaceCapabilities.minImageExtent.width, vkglobals.surfaceCapabilities.maxImageExtent.width);
        clamp(&vkglobals.swapchainExtent.height, vkglobals.surfaceCapabilities.minImageExtent.height, vkglobals.surfaceCapabilities.maxImageExtent.height);
    }

    {
        u32 swapchainImageCount;
        if (vkglobals.surfaceCapabilities.minImageCount + 1 <= vkglobals.surfaceCapabilities.maxImageCount) {
            swapchainImageCount = vkglobals.surfaceCapabilities.minImageCount + 1;
        } else {
            swapchainImageCount = vkglobals.surfaceCapabilities.minImageCount;
        }

        VkSwapchainCreateInfoKHR swapchainInfo = {};
        swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainInfo.clipped = VK_TRUE;
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.imageFormat = vkglobals.surfaceFormat.format;
        swapchainInfo.imageColorSpace = vkglobals.surfaceFormat.colorSpace;
        swapchainInfo.imageExtent = vkglobals.swapchainExtent;
        swapchainInfo.minImageCount = swapchainImageCount;
        swapchainInfo.presentMode = vkglobals.surfacePresentMode;
        swapchainInfo.preTransform = vkglobals.surfaceCapabilities.currentTransform;
        swapchainInfo.surface = vkglobals.surface;

        VK_ASSERT(vkCreateSwapchainKHR(vkglobals.device, &swapchainInfo, VK_NULL_HANDLE, &vkglobals.swapchain), "failed to create swapchain\n");
    }

    vkglobals.loopActive = 1;
    vkglobals.deltaTime = 0;
    vkglobals.time = 0;
    vkglobals.fps = 0.0f;
}

void vkQuit() {
    vkDestroySwapchainKHR(vkglobals.device, vkglobals.swapchain, VK_NULL_HANDLE);
    vkDestroyCommandPool(vkglobals.device, vkglobals.shortCommandPool, VK_NULL_HANDLE);
    vkDestroyCommandPool(vkglobals.device, vkglobals.commandPool, VK_NULL_HANDLE);
    vkDestroyDevice(vkglobals.device, VK_NULL_HANDLE);
    SDL_Vulkan_DestroySurface(vkglobals.instance, vkglobals.surface, VK_NULL_HANDLE);
    vkDestroyInstance(vkglobals.instance, VK_NULL_HANDLE);
}