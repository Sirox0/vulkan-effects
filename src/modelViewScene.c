#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <cglm/cglm.h>
#include <stb_image.h>
#include <ktx.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>
#include <math.h>
#include <vulkan/vulkan_core.h>

#include "cglm/affine-pre.h"
#include "numtypes.h"
#include "vkFunctions.h"
#include "vk.h"
#include "pipeline.h"
#include "modelViewScene.h"
#include "util.h"
#include "temp.h"
#include "config.h"
#include "mathext.h"
#include "vkModel.h"
#include "scene.h"

#define globals ((ModelViewSceneGlobals_t*)curscene.globals)

#define SHADOWMAP_RESOLUTION 5

void modelViewSceneInit() {
    VkCommandBuffer tempCmdBuffer;
    VkBuffer tempBuffer;
    VkDeviceMemory tempBufferMemory;
    VkFence tempFence;

    tempResourcesCreate(1, &tempCmdBuffer, 1, &tempFence);

    {
        VkCommandBufferAllocateInfo cmdBufferInfo = {};
        cmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufferInfo.commandBufferCount = 1;
        cmdBufferInfo.commandPool = vkglobals.commandPool;

        VK_ASSERT(vkAllocateCommandBuffers(vkglobals.device, &cmdBufferInfo, &globals->cmdBuffer), "failed to allocate command buffer\n");
    }

    {
        vkGetSwapchainImagesKHR(vkglobals.device, vkglobals.swapchain, &globals->swapchainImageCount, VK_NULL_HANDLE);

        globals->swapchainImages = (VkImage*)malloc((sizeof(VkImage) + sizeof(VkImageView)) * globals->swapchainImageCount);
        globals->swapchainImageViews = (VkImageView*)(((void*)globals->swapchainImages) + sizeof(VkImage) * globals->swapchainImageCount);

        vkGetSwapchainImagesKHR(vkglobals.device, vkglobals.swapchain, &globals->swapchainImageCount, globals->swapchainImages);

        for (u32 i = 0; i < globals->swapchainImageCount; i++) {
            createImageView(&globals->swapchainImageViews[i], globals->swapchainImages[i], VK_IMAGE_VIEW_TYPE_2D, vkglobals.surfaceFormat.format, 1, 0, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT);
        }
    }

    globals->pipelineCache = loadPipelineCache("pipelinecache.dat");

    {
        #define DEPTH_FORMAT_COUNT 3
        VkFormat formats[DEPTH_FORMAT_COUNT] = {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT};
        
        globals->depthFormat = VK_FORMAT_UNDEFINED;
        for (u32 i = 0; i < DEPTH_FORMAT_COUNT; i++) {
            VkFormatProperties2KHR props;
            props.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2_KHR;
            props.pNext = VK_NULL_HANDLE;

            vkGetPhysicalDeviceFormatProperties2KHR(vkglobals.physicalDevice, formats[i], &props);
            if (props.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                globals->depthFormat = formats[i];
            }
        }
        if (globals->depthFormat == VK_FORMAT_UNDEFINED) {
            printf("failed to find suitable depth texture format\n");
            exit(1);
        }
    }

    {
        struct {
            vec3 pos;
        } skyboxVertexbuf[] = {
             // left face
            {{-1000.0f, -1000.0f, -1000.0f}},
            {{-1000.0f, 1000.0f, 1000.0f}},
            {{-1000.0f, -1000.0f, 1000.0f}},
            {{-1000.0f, 1000.0f, 1000.0f}},
            {{-1000.0f, -1000.0f, -1000.0f}},
            {{-1000.0f, 1000.0f, -1000.0f}},
            // back face
            {{-1000.0f, -1000.0f, -1000.0f}},
            {{1000.0f, -1000.0f, -1000.0f}},
            {{1000.0f, 1000.0f, -1000.0f}},
            {{-1000.0f, -1000.0f, -1000.0f}},
            {{1000.0f, 1000.0f, -1000.0f}},
            {{-1000.0f, 1000.0f, -1000.0f}},
            // top face
            {{-1000.0f, -1000.0f, -1000.0f}},
            {{1000.0f, -1000.0f, 1000.0f}},
            {{1000.0f, -1000.0f, -1000.0f}},
            {{-1000.0f, -1000.0f, -1000.0f}},
            {{-1000.0f, -1000.0f, 1000.0f}},
            {{1000.0f, -1000.0f, 1000.0f}},
            // bottom face
            {{-1000.0f, 1000.0f, -1000.0f}},
            {{1000.0f, 1000.0f, 1000.0f}},
            {{-1000.0f, 1000.0f, 1000.0f}},
            {{-1000.0f, 1000.0f, -1000.0f}},
            {{1000.0f, 1000.0f, -1000.0f}},
            {{1000.0f, 1000.0f, 1000.0f}},
            // right face
            {{1000.0f, 1000.0f, -1000.0f}},
            {{1000.0f, -1000.0f, 1000.0f}},
            {{1000.0f, 1000.0f, 1000.0f}},
            {{1000.0f, -1000.0f, 1000.0f}},
            {{1000.0f, 1000.0f, -1000.0f}},
            {{1000.0f, -1000.0f, -1000.0f}},
            // front face
            {{-1000.0f, 1000.0f, 1000.0f}},
            {{1000.0f, 1000.0f, 1000.0f}},
            {{-1000.0f, -1000.0f, 1000.0f}},
            {{-1000.0f, -1000.0f, 1000.0f}},
            {{1000.0f, 1000.0f, 1000.0f}},
            {{1000.0f, -1000.0f, 1000.0f}}
        };

        char modelPath[256 + 64] = {};
        strcat(modelPath, config.modelDirectoryPath);
        strcat(modelPath, config.modelFile);

        const struct aiScene* scene = vkModelLoadScene(modelPath);

        u32 imagesSize, imageCount;
        vkModelGetTexturesInfo(scene, config.modelDirectoryPath, &imagesSize, &imageCount, NULL, NULL, NULL, NULL);
        u32 imageMipLevels[imageCount], imageWidths[imageCount], imageHeights[imageCount];
        VkFormat imageFormats[imageCount];
        vkModelGetTexturesInfo(scene, config.modelDirectoryPath, &imagesSize, &imageCount, imageMipLevels, imageFormats, imageWidths, imageHeights);

        globals->model.textures = (VkImage*)malloc((sizeof(VkImage) + sizeof(VkImageView)) * imageCount);
        globals->model.views = (VkImageView*)(((void*)globals->model.textures) + sizeof(VkImage) * imageCount);

        u32 vertexSize, indexSize, indirectSize, storageMaterialsSize, storageTransformsSize, storageMeshIndicesSize;
        vkModelGetSizes(scene, &vertexSize, &indexSize, &indirectSize, &storageMaterialsSize, &storageTransformsSize, &storageMeshIndicesSize);

        globals->model.materialsSize = storageMaterialsSize;
        globals->model.transformsSize = storageTransformsSize;
        globals->model.transformsOffset = storageMaterialsSize + getAlignCooficient(storageMaterialsSize, vkglobals.deviceProperties.properties.limits.minStorageBufferOffsetAlignment);
        globals->model.meshIndicesOffset = globals->model.transformsOffset + storageTransformsSize + getAlignCooficient(globals->model.transformsOffset + storageTransformsSize, vkglobals.deviceProperties.properties.limits.minStorageBufferOffsetAlignment);

        {
            // device-local resources
            createImage(&globals->depthTexture, vkglobals.swapchainExtent.width, vkglobals.swapchainExtent.height, globals->depthFormat, 1, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0);
            createImage(&globals->shadowmap, vkglobals.swapchainExtent.width * SHADOWMAP_RESOLUTION, vkglobals.swapchainExtent.height * SHADOWMAP_RESOLUTION, globals->depthFormat, 1, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0);

            for (u32 i = 0; i < imageCount; i++) createImage(&globals->model.textures[i], imageWidths[i], imageHeights[i], imageFormats[i], 1, imageMipLevels[i], VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 0);
            
            createBuffer(&globals->projectionBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, sizeof(mat4) * 2 + sizeof(f32) * 2);

            createBuffer(&globals->model.storageBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, globals->model.meshIndicesOffset + storageMeshIndicesSize);

            createBuffer(&globals->skyboxVertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, sizeof(skyboxVertexbuf));
            createBuffer(&globals->model.vertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vertexSize);

            createBuffer(&globals->model.indexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, indexSize);

            createBuffer(&globals->model.indirectBuffer, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, indirectSize);
            
            createImage(&globals->gbuffer, vkglobals.swapchainExtent.width, vkglobals.swapchainExtent.height, VK_FORMAT_R8G8B8A8_UNORM, 2, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0);
            createImage(&globals->metallicRoughnessVelocityTexture, vkglobals.swapchainExtent.width, vkglobals.swapchainExtent.height, VK_FORMAT_R16G16B16A16_SFLOAT, 1, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0);
            createImage(&globals->ssaoAttachment, config.ssaoResolutionWidth, config.ssaoResolutionHeight, VK_FORMAT_R8_UNORM, 1, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0);
            createImage(&globals->postProcessAttachment, vkglobals.swapchainExtent.width, vkglobals.swapchainExtent.height, VK_FORMAT_R16G16B16A16_SFLOAT, 1, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0);
            createImage(&globals->blurredPostProcessAttachment, vkglobals.swapchainExtent.width, vkglobals.swapchainExtent.height, VK_FORMAT_R16G16B16A16_SFLOAT, 1, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0);

            {
                VkMemoryAllocClusterInfo_t allocInfo = {};
                allocInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                allocInfo.handleType = VK_MEMORY_ALLOC_CLUSTER_HANDLE_TYPE_IMAGE;
                allocInfo.handleCount = 2;
                allocInfo.pHandles = (VkImage[]){globals->depthTexture, globals->shadowmap};
                allocInfo.pDstOffsets = VK_NULL_HANDLE;

                vkAllocateMemoryCluster(&allocInfo, &globals->deviceLocalDepthStencilAttachmentMemory);
            }
            
            {
                VkImage images[imageCount];
                for (u32 i = 0; i < imageCount; i++) images[i] = globals->model.textures[i];

                VkMemoryAllocClusterInfo_t allocInfo = {};
                allocInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                allocInfo.handleType = VK_MEMORY_ALLOC_CLUSTER_HANDLE_TYPE_IMAGE;
                allocInfo.handleCount = imageCount;
                allocInfo.pHandles = images;
                allocInfo.pDstOffsets = VK_NULL_HANDLE;

                vkAllocateMemoryCluster(&allocInfo, &globals->deviceLocalSampledTransferDstMemory);
            }

            {
                VkMemoryAllocClusterInfo_t allocInfo = {};
                allocInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                allocInfo.handleType = VK_MEMORY_ALLOC_CLUSTER_HANDLE_TYPE_BUFFER;
                allocInfo.handleCount = 1;
                allocInfo.pHandles = (VkBuffer[]){globals->projectionBuffer};
                allocInfo.pDstOffsets = VK_NULL_HANDLE;

                vkAllocateMemoryCluster(&allocInfo, &globals->deviceLocalUniformTransferDstMemory);
            }

            {
                VkMemoryAllocClusterInfo_t allocInfo = {};
                allocInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                allocInfo.handleType = VK_MEMORY_ALLOC_CLUSTER_HANDLE_TYPE_BUFFER;
                allocInfo.handleCount = 1;
                allocInfo.pHandles = &globals->model.storageBuffer;
                allocInfo.pDstOffsets = VK_NULL_HANDLE;

                vkAllocateMemoryCluster(&allocInfo, &globals->deviceLocalStorageTransferDstMemory);
            }

            {
                VkMemoryAllocClusterInfo_t allocInfo = {};
                allocInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                allocInfo.handleType = VK_MEMORY_ALLOC_CLUSTER_HANDLE_TYPE_BUFFER;
                allocInfo.handleCount = 2;
                allocInfo.pHandles = (VkBuffer[]){globals->skyboxVertexBuffer, globals->model.vertexBuffer};
                allocInfo.pDstOffsets = VK_NULL_HANDLE;

                vkAllocateMemoryCluster(&allocInfo, &globals->deviceLocalVertexTransferDstMemory);
            }

            {
                VkMemoryAllocClusterInfo_t allocInfo = {};
                allocInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                allocInfo.handleType = VK_MEMORY_ALLOC_CLUSTER_HANDLE_TYPE_BUFFER;
                allocInfo.handleCount = 1;
                allocInfo.pHandles = &globals->model.indexBuffer;
                allocInfo.pDstOffsets = VK_NULL_HANDLE;

                vkAllocateMemoryCluster(&allocInfo, &globals->deviceLocalIndexTransferDstMemory);
            }

            {
                VkMemoryAllocClusterInfo_t allocInfo = {};
                allocInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                allocInfo.handleType = VK_MEMORY_ALLOC_CLUSTER_HANDLE_TYPE_BUFFER;
                allocInfo.handleCount = 1;
                allocInfo.pHandles = &globals->model.indirectBuffer;
                allocInfo.pDstOffsets = VK_NULL_HANDLE;

                vkAllocateMemoryCluster(&allocInfo, &globals->deviceLocalIndirectTransferDstMemory);
            }

            {
                VkMemoryAllocClusterInfo_t allocInfo = {};
                allocInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                allocInfo.handleType = VK_MEMORY_ALLOC_CLUSTER_HANDLE_TYPE_IMAGE;
                allocInfo.handleCount = 5;
                allocInfo.pHandles = (VkImage[]){globals->gbuffer, globals->metallicRoughnessVelocityTexture, globals->ssaoAttachment, globals->postProcessAttachment, globals->blurredPostProcessAttachment};
                allocInfo.pDstOffsets = VK_NULL_HANDLE;

                vkAllocateMemoryCluster(&allocInfo, &globals->deviceLocalColorAttachmentSampledMemory);
            }



            // host-visible resources
            createBuffer(&globals->viewMatrixBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(mat4) * 3);

            {
                VkMemoryAllocClusterInfo_t allocInfo = {};
                allocInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                allocInfo.handleType = VK_MEMORY_ALLOC_CLUSTER_HANDLE_TYPE_BUFFER;
                allocInfo.handleCount = 1;
                allocInfo.pHandles = (VkBuffer[]){globals->viewMatrixBuffer};
                allocInfo.pDstOffsets = VK_NULL_HANDLE;

                vkAllocateMemoryCluster(&allocInfo, &globals->hostVisibleUniformMemory);

                VK_ASSERT(vkMapMemory(vkglobals.device, globals->hostVisibleUniformMemory, 0, VK_WHOLE_SIZE, 0, &globals->hostVisibleUniformMemoryRaw), "failed to map memory\n");
            }

            createBuffer(&globals->model.hostVisibleStorageBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(mat4));
            createBuffer(&globals->lightBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(mat4) + sizeof(vec4));

            {
                VkDeviceSize offsets[2];

                VkMemoryAllocClusterInfo_t allocInfo = {};
                allocInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                allocInfo.handleType = VK_MEMORY_ALLOC_CLUSTER_HANDLE_TYPE_BUFFER;
                allocInfo.handleCount = 2;
                allocInfo.pHandles = (VkBuffer[]){globals->model.hostVisibleStorageBuffer, globals->lightBuffer};
                allocInfo.pDstOffsets = offsets;

                vkAllocateMemoryCluster(&allocInfo, &globals->hostVisibleStorageMemory);

                globals->lightBufferOffset = offsets[1];

                VK_ASSERT(vkMapMemory(vkglobals.device, globals->hostVisibleStorageMemory, 0, VK_WHOLE_SIZE, 0, &globals->hostVisibleStorageMemoryRaw), "failed to map memory\n");
            }



            // temporary resources
            createBuffer(&tempBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, sizeof(skyboxVertexbuf) +
                sizeof(mat4) * 2 + sizeof(f32) * 2 + vertexSize + indexSize + indirectSize +
                storageMaterialsSize + storageTransformsSize + storageMeshIndicesSize + imagesSize
            );
            
            {
                VkMemoryAllocClusterInfo_t allocInfo = {};
                allocInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                allocInfo.handleType = VK_MEMORY_ALLOC_CLUSTER_HANDLE_TYPE_BUFFER;
                allocInfo.handleCount = 1;
                allocInfo.pHandles = &tempBuffer;
                allocInfo.pDstOffsets = VK_NULL_HANDLE;

                vkAllocateMemoryCluster(&allocInfo, &tempBufferMemory);
            }
        }

        for (u32 i = 0; i < imageCount; i++) createImageView(&globals->model.views[i], globals->model.textures[i], VK_IMAGE_VIEW_TYPE_2D, imageFormats[i], 1, 0, imageMipLevels[i], 0, VK_IMAGE_ASPECT_COLOR_BIT);

        {
            #define tempBufferMemRawProjectionMatrixoffset (sizeof(skyboxVertexbuf))
            #define tempBufferMemRawModeloffset (tempBufferMemRawProjectionMatrixoffset + sizeof(mat4) * 2 + sizeof(f32) * 2)

            {
                void* tempBufferMemRaw;
                VK_ASSERT(vkMapMemory(vkglobals.device, tempBufferMemory, 0, VK_WHOLE_SIZE, 0, &tempBufferMemRaw), "failed to map memory\n");

                memcpy(tempBufferMemRaw, skyboxVertexbuf, sizeof(skyboxVertexbuf));

                // use reverse depth
                glm_perspective(glm_rad(config.fov), (f32)vkglobals.swapchainExtent.width / vkglobals.swapchainExtent.height, config.farPlane, config.nearPlane, tempBufferMemRaw + tempBufferMemRawProjectionMatrixoffset);
                glm_mat4_inv(tempBufferMemRaw + tempBufferMemRawProjectionMatrixoffset, tempBufferMemRaw + tempBufferMemRawProjectionMatrixoffset + sizeof(mat4));
                memcpy(tempBufferMemRaw + tempBufferMemRawProjectionMatrixoffset + sizeof(mat4) * 2, &config.nearPlane, sizeof(f32));
                memcpy(tempBufferMemRaw + tempBufferMemRawProjectionMatrixoffset + sizeof(mat4) * 2 + sizeof(f32), &config.farPlane, sizeof(f32));

                vkModelCreate(scene, config.modelDirectoryPath, tempCmdBuffer, tempBuffer, 
                    tempBufferMemRawModeloffset,
                    tempBufferMemRawModeloffset + vertexSize,
                    tempBufferMemRawModeloffset + vertexSize + indexSize,
                    tempBufferMemRawModeloffset + vertexSize + indexSize + indirectSize,
                    tempBufferMemRawModeloffset + vertexSize + indexSize + indirectSize + storageMaterialsSize,
                    tempBufferMemRawModeloffset + vertexSize + indexSize + indirectSize + storageMaterialsSize + storageTransformsSize,
                    tempBufferMemRawModeloffset + vertexSize + indexSize + indirectSize + storageMaterialsSize + storageTransformsSize + storageMeshIndicesSize,
                    globals->model.transformsOffset, globals->model.meshIndicesOffset,
                    tempBufferMemRaw, &globals->model
                );

                VkMappedMemoryRange memoryRange = {};
                memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
                memoryRange.offset = 0;
                memoryRange.size = VK_WHOLE_SIZE;
                memoryRange.memory = tempBufferMemory;

                VK_ASSERT(vkFlushMappedMemoryRanges(vkglobals.device, 1, &memoryRange), "failed to flush device memory\n");
            }
            
            vkUnmapMemory(vkglobals.device, tempBufferMemory);

            {
                VkBufferCopy copyInfos[2] = {};
                copyInfos[0].srcOffset = 0;
                copyInfos[0].dstOffset = 0;
                copyInfos[0].size = sizeof(skyboxVertexbuf);

                copyInfos[1].srcOffset = tempBufferMemRawProjectionMatrixoffset;
                copyInfos[1].dstOffset = 0;
                copyInfos[1].size = sizeof(mat4) * 2 + sizeof(f32) * 2;

                vkCmdCopyBuffer(tempCmdBuffer, tempBuffer, globals->skyboxVertexBuffer, 1, copyInfos);
                vkCmdCopyBuffer(tempCmdBuffer, tempBuffer, globals->projectionBuffer, 1, copyInfos + 1);
            }
        }

        vkModelUnloadScene(scene);
    }

    {
        VkImageMemoryBarrier imageBarriers[2] = {};
        imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarriers[0].image = globals->depthTexture;
        imageBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        imageBarriers[0].subresourceRange.baseArrayLayer = 0;
        imageBarriers[0].subresourceRange.layerCount = 1;
        imageBarriers[0].subresourceRange.baseMipLevel = 0;
        imageBarriers[0].subresourceRange.levelCount = 1;
        imageBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarriers[0].srcAccessMask = 0;
        imageBarriers[0].dstAccessMask = 0;
        imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        if (!config.bloomEnable) {
            imageBarriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarriers[1].image = globals->blurredPostProcessAttachment;
            imageBarriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarriers[1].subresourceRange.baseArrayLayer = 0;
            imageBarriers[1].subresourceRange.layerCount = 1;
            imageBarriers[1].subresourceRange.baseMipLevel = 0;
            imageBarriers[1].subresourceRange.levelCount = 1;
            imageBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[1].srcAccessMask = 0;
            imageBarriers[1].dstAccessMask = 0;
            imageBarriers[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageBarriers[1].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        vkCmdPipelineBarrier(tempCmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, config.bloomEnable ? 1 : 2, imageBarriers);
    }

    {
        VK_ASSERT(vkEndCommandBuffer(tempCmdBuffer), "failed to end command buffer\n");

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &tempCmdBuffer;

        VK_ASSERT(vkQueueSubmit(vkglobals.queue, 1, &submitInfo, tempFence), "failed to submit command buffer\n");
    }

    createImageView(&globals->depthTextureView, globals->depthTexture, VK_IMAGE_VIEW_TYPE_2D, globals->depthFormat, 1, 0, 1, 0, VK_IMAGE_ASPECT_DEPTH_BIT);
    createImageView(&globals->shadowmapView, globals->shadowmap, VK_IMAGE_VIEW_TYPE_2D, globals->depthFormat, 1, 0, 1, 0, VK_IMAGE_ASPECT_DEPTH_BIT);
    createImageView(&globals->gbufferNormalView, globals->gbuffer, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, 1, 0, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT);
    createImageView(&globals->gbufferAlbedoView, globals->gbuffer, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT);
    createImageView(&globals->metallicRoughnessVelocityTextureView, globals->metallicRoughnessVelocityTexture, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R16G16B16A16_SFLOAT, 1, 0, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT);
    createImageView(&globals->ssaoAttachmentView, globals->ssaoAttachment, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8_UNORM, 1, 0, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT);
    createImageView(&globals->postProcessAttachmentView, globals->postProcessAttachment, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R16G16B16A16_SFLOAT, 1, 0, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT);
    createImageView(&globals->blurredPostProcessAttachmentView, globals->blurredPostProcessAttachment, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R16G16B16A16_SFLOAT, 1, 0, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT);
    
    {
        {
            VkSamplerCreateInfo samplerInfo = {};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
            samplerInfo.minFilter = vkglobals.textureFilter;
            samplerInfo.magFilter = vkglobals.textureFilter;
            if (config.maxAnisotropy) samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = config.maxAnisotropy;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

            VK_ASSERT(vkCreateSampler(vkglobals.device, &samplerInfo, VK_NULL_HANDLE, &globals->sampler), "failed to create sampler\n");
        }

        {
            VkSamplerCreateInfo samplerInfo = {};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.anisotropyEnable = VK_FALSE;
            samplerInfo.compareEnable = VK_TRUE;
            samplerInfo.compareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
            samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

            VK_ASSERT(vkCreateSampler(vkglobals.device, &samplerInfo, VK_NULL_HANDLE, &globals->shadowmapSampler), "failed to create sampler\n");
        }
    }

    {
        VkDescriptorPoolSize poolSizes[4] = {};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = 3;

        poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[1].descriptorCount = 5;

        poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[2].descriptorCount = 1 + globals->model.textureCount;

        poolSizes[3].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        poolSizes[3].descriptorCount = 7;

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 4;
        poolInfo.pPoolSizes = poolSizes;
        poolInfo.maxSets = 10;

        VK_ASSERT(vkCreateDescriptorPool(vkglobals.device, &poolInfo, VK_NULL_HANDLE, &globals->descriptorPool), "failed to create descriptor pool\n");
    }

    {
        {
            VkDescriptorSetLayoutBinding bindings[1] = {};
            bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings[0].binding = 0;
            bindings[0].descriptorCount = 1;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
            descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutInfo.bindingCount = 1;
            descriptorSetLayoutInfo.pBindings = bindings;

            VK_ASSERT(vkCreateDescriptorSetLayout(vkglobals.device, &descriptorSetLayoutInfo, VK_NULL_HANDLE, &globals->storageDescriptorSetLayout), "failed to create descriptor set layout\n");
        }

        {
            VkDescriptorSetLayoutBinding bindings[5] = {};
            bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            bindings[0].binding = 0;
            bindings[0].descriptorCount = 1;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

            bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            bindings[1].binding = 1;
            bindings[1].descriptorCount = 1;
            bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

            bindings[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            bindings[2].binding = 2;
            bindings[2].descriptorCount = 1;
            bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

            bindings[3].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            bindings[3].binding = 3;
            bindings[3].descriptorCount = 1;
            bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            
            bindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings[4].binding = 4;
            bindings[4].descriptorCount = globals->model.textureCount;
            bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

            VkDescriptorBindingFlags bindingFlagsValues[5] = {};
            bindingFlagsValues[4] = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

            VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlags = {};
            bindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
            bindingFlags.bindingCount = 5;
            bindingFlags.pBindingFlags = bindingFlagsValues;

            VkDescriptorSetLayoutCreateInfo layoutInfo = {};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.pNext = &bindingFlags;
            layoutInfo.bindingCount = 5;
            layoutInfo.pBindings = bindings;

            VK_ASSERT(vkCreateDescriptorSetLayout(vkglobals.device, &layoutInfo, VK_NULL_HANDLE, &globals->modelDescriptorSetLayout), "failed to create descriptor set layout\n");
        }

        {
            VkDescriptorSetLayoutBinding bindings[1] = {};
            bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            bindings[0].binding = 0;
            bindings[0].descriptorCount = 1;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
            descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutInfo.bindingCount = 1;
            descriptorSetLayoutInfo.pBindings = bindings;

            VK_ASSERT(vkCreateDescriptorSetLayout(vkglobals.device, &descriptorSetLayoutInfo, VK_NULL_HANDLE, &globals->uniformDescriptorSetLayout), "failed to create descriptor set layout\n");
        }

        {
            VkDescriptorSetLayoutBinding bindings[1] = {};
            bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings[0].binding = 0;
            bindings[0].descriptorCount = 4;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
            descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutInfo.bindingCount = 1;
            descriptorSetLayoutInfo.pBindings = bindings;

            VK_ASSERT(vkCreateDescriptorSetLayout(vkglobals.device, &descriptorSetLayoutInfo, VK_NULL_HANDLE, &globals->gbufferDescriptorSetLayout), "failed to create descriptor set layout\n");
        }

        {
            VkDescriptorSetLayoutBinding bindings[2] = {};
            bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings[0].binding = 0;
            bindings[0].descriptorCount = 1;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

            bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings[1].binding = 1;
            bindings[1].descriptorCount = 1;
            bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
            descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutInfo.bindingCount = 2;
            descriptorSetLayoutInfo.pBindings = bindings;

            VK_ASSERT(vkCreateDescriptorSetLayout(vkglobals.device, &descriptorSetLayoutInfo, VK_NULL_HANDLE, &globals->PVmatrixdescriptorSetLayout), "failed to create descriptor set layout\n");
        }

        {
            VkDescriptorSetLayoutBinding bindings[1] = {};
            bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings[0].binding = 0;
            bindings[0].descriptorCount = 1;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
            descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutInfo.bindingCount = 1;
            descriptorSetLayoutInfo.pBindings = bindings;

            VK_ASSERT(vkCreateDescriptorSetLayout(vkglobals.device, &descriptorSetLayoutInfo, VK_NULL_HANDLE, &globals->sampledImageDescriptorSetLayout), "failed to create descriptor set layout\n");
        }

        {
            VkDescriptorSetLayoutBinding bindings[1] = {};
            bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings[0].binding = 0;
            bindings[0].descriptorCount = 1;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
            descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutInfo.bindingCount = 1;
            descriptorSetLayoutInfo.pBindings = bindings;

            VK_ASSERT(vkCreateDescriptorSetLayout(vkglobals.device, &descriptorSetLayoutInfo, VK_NULL_HANDLE, &globals->combinedImageSamplerDescriptorSetLayout), "failed to create descriptor set layout\n");
        }
    }

    {
        {
            u32 variableDescriptorCounts[8] = {};
            variableDescriptorCounts[4] = globals->model.textureCount;

            VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variableDescriptorCountAllocateInfo = {};
            variableDescriptorCountAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
            variableDescriptorCountAllocateInfo.descriptorSetCount = 8;
            variableDescriptorCountAllocateInfo.pDescriptorCounts = variableDescriptorCounts;

            VkDescriptorSetAllocateInfo descriptorSetsInfo = {};
            descriptorSetsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            descriptorSetsInfo.pNext = &variableDescriptorCountAllocateInfo;
            descriptorSetsInfo.descriptorPool = globals->descriptorPool;
            descriptorSetsInfo.descriptorSetCount = 8;
            descriptorSetsInfo.pSetLayouts = (VkDescriptorSetLayout[]){globals->PVmatrixdescriptorSetLayout, globals->gbufferDescriptorSetLayout,
                globals->sampledImageDescriptorSetLayout, globals->sampledImageDescriptorSetLayout, globals->modelDescriptorSetLayout,
                globals->storageDescriptorSetLayout, globals->combinedImageSamplerDescriptorSetLayout, globals->sampledImageDescriptorSetLayout
            };

            VkDescriptorSet sets[8];

            VK_ASSERT(vkAllocateDescriptorSets(vkglobals.device, &descriptorSetsInfo, sets), "failed to allocate descriptor sets\n");

            globals->PVmatrixdescriptorSet = sets[0];
            globals->gbufferDescriptorSet = sets[1];
            globals->ssaoAttachmentDescriptorSet = sets[2];
            globals->postProcessAttachmentDescriptorSet = sets[3];
            globals->model.descriptorSet = sets[4];
            globals->lightDescriptorSet = sets[5];
            globals->shadowmapDescriptorSet = sets[6];
            globals->blurredPostProcessAttachmentDescriptorSet = sets[7];
        }

        u32 modelDescriptorBufferCount, modelDescriptorImageCount, modelDescriptorWriteCount;
        vkModelGetDescriptorWrites(&globals->model, globals->sampler, &modelDescriptorBufferCount, NULL, &modelDescriptorImageCount, NULL, &modelDescriptorWriteCount, NULL);

        VkDescriptorBufferInfo descriptorBufferInfos[3 + modelDescriptorBufferCount];
        descriptorBufferInfos[0].buffer = globals->projectionBuffer;
        descriptorBufferInfos[0].offset = 0;
        descriptorBufferInfos[0].range = VK_WHOLE_SIZE;

        descriptorBufferInfos[1].buffer = globals->viewMatrixBuffer;
        descriptorBufferInfos[1].offset = 0;
        descriptorBufferInfos[1].range = VK_WHOLE_SIZE;

        descriptorBufferInfos[2].buffer = globals->lightBuffer;
        descriptorBufferInfos[2].offset = 0;
        descriptorBufferInfos[2].range = VK_WHOLE_SIZE;

        VkDescriptorImageInfo descriptorImageInfos[8 + modelDescriptorImageCount];
        descriptorImageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfos[0].imageView = globals->gbufferNormalView;
        descriptorImageInfos[0].sampler = VK_NULL_HANDLE;

        descriptorImageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfos[1].imageView = globals->gbufferAlbedoView;
        descriptorImageInfos[1].sampler = VK_NULL_HANDLE;

        descriptorImageInfos[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfos[2].imageView = globals->metallicRoughnessVelocityTextureView;
        descriptorImageInfos[2].sampler = VK_NULL_HANDLE;

        descriptorImageInfos[3].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfos[3].imageView = globals->depthTextureView;
        descriptorImageInfos[3].sampler = VK_NULL_HANDLE;

        descriptorImageInfos[4].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfos[4].imageView = globals->ssaoAttachmentView;
        descriptorImageInfos[4].sampler = VK_NULL_HANDLE;

        descriptorImageInfos[5].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfos[5].imageView = globals->postProcessAttachmentView;
        descriptorImageInfos[5].sampler = VK_NULL_HANDLE;

        descriptorImageInfos[6].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfos[6].imageView = globals->shadowmapView;
        descriptorImageInfos[6].sampler = globals->shadowmapSampler;

        descriptorImageInfos[7].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfos[7].imageView = globals->blurredPostProcessAttachmentView;
        descriptorImageInfos[7].sampler = VK_NULL_HANDLE;

        VkWriteDescriptorSet descriptorWrites[11 + modelDescriptorWriteCount];
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].pNext = VK_NULL_HANDLE;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].dstSet = globals->PVmatrixdescriptorSet;
        descriptorWrites[0].pBufferInfo = &descriptorBufferInfos[0];

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].pNext = VK_NULL_HANDLE;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].dstSet = globals->PVmatrixdescriptorSet;
        descriptorWrites[1].pBufferInfo = &descriptorBufferInfos[1];

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].pNext = VK_NULL_HANDLE;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrites[2].dstBinding = 0;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].dstSet = globals->gbufferDescriptorSet;
        descriptorWrites[2].pImageInfo = &descriptorImageInfos[0];

        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].pNext = VK_NULL_HANDLE;
        descriptorWrites[3].descriptorCount = 1;
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrites[3].dstBinding = 0;
        descriptorWrites[3].dstArrayElement = 1;
        descriptorWrites[3].dstSet = globals->gbufferDescriptorSet;
        descriptorWrites[3].pImageInfo = &descriptorImageInfos[1];

        descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[4].pNext = VK_NULL_HANDLE;
        descriptorWrites[4].descriptorCount = 1;
        descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrites[4].dstBinding = 0;
        descriptorWrites[4].dstArrayElement = 2;
        descriptorWrites[4].dstSet = globals->gbufferDescriptorSet;
        descriptorWrites[4].pImageInfo = &descriptorImageInfos[2];

        descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[5].pNext = VK_NULL_HANDLE;
        descriptorWrites[5].descriptorCount = 1;
        descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrites[5].dstBinding = 0;
        descriptorWrites[5].dstArrayElement = 3;
        descriptorWrites[5].dstSet = globals->gbufferDescriptorSet;
        descriptorWrites[5].pImageInfo = &descriptorImageInfos[3];

        descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[6].pNext = VK_NULL_HANDLE;
        descriptorWrites[6].descriptorCount = 1;
        descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrites[6].dstBinding = 0;
        descriptorWrites[6].dstArrayElement = 0;
        descriptorWrites[6].dstSet = globals->ssaoAttachmentDescriptorSet;
        descriptorWrites[6].pImageInfo = &descriptorImageInfos[4];

        descriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[7].pNext = VK_NULL_HANDLE;
        descriptorWrites[7].descriptorCount = 1;
        descriptorWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrites[7].dstBinding = 0;
        descriptorWrites[7].dstArrayElement = 0;
        descriptorWrites[7].dstSet = globals->postProcessAttachmentDescriptorSet;
        descriptorWrites[7].pImageInfo = &descriptorImageInfos[5];

        descriptorWrites[8].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[8].pNext = VK_NULL_HANDLE;
        descriptorWrites[8].descriptorCount = 1;
        descriptorWrites[8].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[8].dstBinding = 0;
        descriptorWrites[8].dstArrayElement = 0;
        descriptorWrites[8].dstSet = globals->lightDescriptorSet;
        descriptorWrites[8].pBufferInfo = &descriptorBufferInfos[2];

        descriptorWrites[9].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[9].pNext = VK_NULL_HANDLE;
        descriptorWrites[9].descriptorCount = 1;
        descriptorWrites[9].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[9].dstBinding = 0;
        descriptorWrites[9].dstArrayElement = 0;
        descriptorWrites[9].dstSet = globals->shadowmapDescriptorSet;
        descriptorWrites[9].pImageInfo = &descriptorImageInfos[6];

        descriptorWrites[10].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[10].pNext = VK_NULL_HANDLE;
        descriptorWrites[10].descriptorCount = 1;
        descriptorWrites[10].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrites[10].dstBinding = 0;
        descriptorWrites[10].dstArrayElement = 0;
        descriptorWrites[10].dstSet = globals->blurredPostProcessAttachmentDescriptorSet;
        descriptorWrites[10].pImageInfo = &descriptorImageInfos[7];

        vkModelGetDescriptorWrites(&globals->model, globals->sampler, &modelDescriptorBufferCount, descriptorBufferInfos + 3, &modelDescriptorImageCount, descriptorImageInfos + 8, &modelDescriptorWriteCount, descriptorWrites + 11);

        vkUpdateDescriptorSets(vkglobals.device, 11 + modelDescriptorWriteCount, descriptorWrites, 0, VK_NULL_HANDLE);
    }

    {
        {
            VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 1;
            pipelineLayoutInfo.pSetLayouts = &globals->sampledImageDescriptorSetLayout;

            VK_ASSERT(vkCreatePipelineLayout(vkglobals.device, &pipelineLayoutInfo, VK_NULL_HANDLE, &globals->sampledImagePipelineLayout), "failed to create pipeline layout\n");
        }

        {
            VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 2;
            pipelineLayoutInfo.pSetLayouts = (VkDescriptorSetLayout[]){globals->storageDescriptorSetLayout, globals->modelDescriptorSetLayout};

            VK_ASSERT(vkCreatePipelineLayout(vkglobals.device, &pipelineLayoutInfo, VK_NULL_HANDLE, &globals->shadowmapPipelineLayout), "failed to create pipeline layout\n");
        }

        {
            VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 2;
            pipelineLayoutInfo.pSetLayouts = (VkDescriptorSetLayout[]){globals->PVmatrixdescriptorSetLayout, globals->modelDescriptorSetLayout};

            VK_ASSERT(vkCreatePipelineLayout(vkglobals.device, &pipelineLayoutInfo, VK_NULL_HANDLE, &globals->modelPipelineLayout), "failed to create pipeline layout\n");
        }

        {
            VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 1;
            pipelineLayoutInfo.pSetLayouts = (VkDescriptorSetLayout[]){globals->PVmatrixdescriptorSetLayout};

            VK_ASSERT(vkCreatePipelineLayout(vkglobals.device, &pipelineLayoutInfo, VK_NULL_HANDLE, &globals->PVPipelineLayout), "failed to create pipeline layout\n");
        }

        {
            VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 2;
            pipelineLayoutInfo.pSetLayouts = (VkDescriptorSetLayout[]){globals->PVmatrixdescriptorSetLayout, globals->gbufferDescriptorSetLayout};

            VK_ASSERT(vkCreatePipelineLayout(vkglobals.device, &pipelineLayoutInfo, VK_NULL_HANDLE, &globals->PVgbufferPipelineLayout), "failed to create pipeline layout\n");
        }

        {
            VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 5;
            pipelineLayoutInfo.pSetLayouts = (VkDescriptorSetLayout[]){globals->PVmatrixdescriptorSetLayout, globals->gbufferDescriptorSetLayout, globals->sampledImageDescriptorSetLayout, globals->storageDescriptorSetLayout, globals->combinedImageSamplerDescriptorSetLayout};

            VK_ASSERT(vkCreatePipelineLayout(vkglobals.device, &pipelineLayoutInfo, VK_NULL_HANDLE, &globals->compositionPipelineLayout), "failed to create pipeline layout\n");
        }

        {
            VkPushConstantRange pcRange = {};
            pcRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            pcRange.size = sizeof(f32) * 2;
            pcRange.offset = 0;

            VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 3;
            pipelineLayoutInfo.pSetLayouts = (VkDescriptorSetLayout[]){globals->sampledImageDescriptorSetLayout, globals->gbufferDescriptorSetLayout, globals->sampledImageDescriptorSetLayout};
            pipelineLayoutInfo.pushConstantRangeCount = 1;
            pipelineLayoutInfo.pPushConstantRanges = &pcRange;

            VK_ASSERT(vkCreatePipelineLayout(vkglobals.device, &pipelineLayoutInfo, VK_NULL_HANDLE, &globals->uberPipelineLayout), "failed to create pipeline layout\n");
        }

        VkSpecializationMapEntry ssaoSpecializationMapEntrys[7] = {};
        ssaoSpecializationMapEntrys[0].constantID = 0;
        ssaoSpecializationMapEntrys[0].offset = 0;
        ssaoSpecializationMapEntrys[0].size = sizeof(u32);
        ssaoSpecializationMapEntrys[1].constantID = 1;
        ssaoSpecializationMapEntrys[1].offset = sizeof(u32);
        ssaoSpecializationMapEntrys[1].size = sizeof(f32);
        ssaoSpecializationMapEntrys[2].constantID = 2;
        ssaoSpecializationMapEntrys[2].offset = sizeof(u32) + sizeof(f32);
        ssaoSpecializationMapEntrys[2].size = sizeof(f32);
        ssaoSpecializationMapEntrys[3].constantID = 3;
        ssaoSpecializationMapEntrys[3].offset = sizeof(u32) + sizeof(f32) * 2;
        ssaoSpecializationMapEntrys[3].size = sizeof(f32);
        ssaoSpecializationMapEntrys[4].constantID = 4;
        ssaoSpecializationMapEntrys[4].offset = sizeof(u32) + sizeof(f32) * 3;
        ssaoSpecializationMapEntrys[4].size = sizeof(f32);
        ssaoSpecializationMapEntrys[5].constantID = 5;
        ssaoSpecializationMapEntrys[5].offset = sizeof(u32) + sizeof(f32) * 4;
        ssaoSpecializationMapEntrys[5].size = sizeof(f32);
        ssaoSpecializationMapEntrys[6].constantID = 6;
        ssaoSpecializationMapEntrys[6].offset = sizeof(u32) + sizeof(f32) * 5;
        ssaoSpecializationMapEntrys[6].size = sizeof(f32);

        VkSpecializationMapEntry compositionSpecializationMapEntrys[16] = {};
        compositionSpecializationMapEntrys[0].constantID = 0;
        compositionSpecializationMapEntrys[0].offset = 0;
        compositionSpecializationMapEntrys[0].size = sizeof(i32);
        compositionSpecializationMapEntrys[1].constantID = 1;
        compositionSpecializationMapEntrys[1].offset = sizeof(i32);
        compositionSpecializationMapEntrys[1].size = sizeof(f32);
        compositionSpecializationMapEntrys[2].constantID = 2;
        compositionSpecializationMapEntrys[2].offset = sizeof(i32) + sizeof(f32);
        compositionSpecializationMapEntrys[2].size = sizeof(f32);

        for (u32 i = 3; i < 11; i++) {
            compositionSpecializationMapEntrys[i].constantID = i;
            compositionSpecializationMapEntrys[i].offset = sizeof(i32) + sizeof(f32) * (i-1);
            compositionSpecializationMapEntrys[i].size = sizeof(f32);
        }

        compositionSpecializationMapEntrys[11].constantID = 11;
        compositionSpecializationMapEntrys[11].offset = sizeof(i32) + sizeof(f32) * 10;
        compositionSpecializationMapEntrys[11].size = sizeof(f32);
        compositionSpecializationMapEntrys[12].constantID = 12;
        compositionSpecializationMapEntrys[12].offset = sizeof(i32) + sizeof(f32) * 11;
        compositionSpecializationMapEntrys[12].size = sizeof(u32);

        compositionSpecializationMapEntrys[13].constantID = 13;
        compositionSpecializationMapEntrys[13].offset = sizeof(i32) + sizeof(u32) + sizeof(f32) * 11;
        compositionSpecializationMapEntrys[13].size = sizeof(f32);
        compositionSpecializationMapEntrys[14].constantID = 14;
        compositionSpecializationMapEntrys[14].offset = sizeof(i32) + sizeof(u32) + sizeof(f32) * 12;
        compositionSpecializationMapEntrys[14].size = sizeof(u32);
        compositionSpecializationMapEntrys[15].constantID = 15;
        compositionSpecializationMapEntrys[15].offset = sizeof(i32) + sizeof(u32) * 2 + sizeof(f32) * 12;
        compositionSpecializationMapEntrys[15].size = sizeof(u32);

        VkSpecializationMapEntry gaussianBlurSpecializationMapEntrys[2] = {};
        gaussianBlurSpecializationMapEntrys[0].constantID = 0;
        gaussianBlurSpecializationMapEntrys[0].offset = 0;
        gaussianBlurSpecializationMapEntrys[0].size = sizeof(f32);
        gaussianBlurSpecializationMapEntrys[1].constantID = 1;
        gaussianBlurSpecializationMapEntrys[1].offset = sizeof(f32);
        gaussianBlurSpecializationMapEntrys[1].size = sizeof(f32);

        VkSpecializationMapEntry uberSpecializationMapEntrys[34] = {};
        uberSpecializationMapEntrys[0].constantID = 0;
        uberSpecializationMapEntrys[0].offset = 0;
        uberSpecializationMapEntrys[0].size = sizeof(f32);

        for (u32 i = 1; i < 19; i++) {
            uberSpecializationMapEntrys[i].constantID = i;
            uberSpecializationMapEntrys[i].offset = sizeof(f32) * i;
            uberSpecializationMapEntrys[i].size = sizeof(f32);
        }

        uberSpecializationMapEntrys[19].constantID = 19;
        uberSpecializationMapEntrys[19].offset = sizeof(f32) * 19;
        uberSpecializationMapEntrys[19].size = sizeof(u32);
        uberSpecializationMapEntrys[20].constantID = 20;
        uberSpecializationMapEntrys[20].offset = sizeof(u32) + sizeof(f32) * 19;
        uberSpecializationMapEntrys[20].size = sizeof(f32);
        uberSpecializationMapEntrys[21].constantID = 21;
        uberSpecializationMapEntrys[21].offset = sizeof(u32) + sizeof(f32) * 20;
        uberSpecializationMapEntrys[21].size = sizeof(f32);
        uberSpecializationMapEntrys[22].constantID = 22;
        uberSpecializationMapEntrys[22].offset = sizeof(u32) + sizeof(f32) * 21;
        uberSpecializationMapEntrys[22].size = sizeof(f32);

        uberSpecializationMapEntrys[23].constantID = 23;
        uberSpecializationMapEntrys[23].offset = sizeof(u32) + sizeof(f32) * 22;
        uberSpecializationMapEntrys[23].size = sizeof(u32);
        uberSpecializationMapEntrys[24].constantID = 24;
        uberSpecializationMapEntrys[24].offset = sizeof(u32) * 2 + sizeof(f32) * 22;
        uberSpecializationMapEntrys[24].size = sizeof(f32);

        uberSpecializationMapEntrys[25].constantID = 25;
        uberSpecializationMapEntrys[25].offset = sizeof(u32) * 2 + sizeof(f32) * 23;
        uberSpecializationMapEntrys[25].size = sizeof(u32);
        uberSpecializationMapEntrys[26].constantID = 26;
        uberSpecializationMapEntrys[26].offset = sizeof(u32) * 3 + sizeof(f32) * 23;
        uberSpecializationMapEntrys[26].size = sizeof(f32);
        uberSpecializationMapEntrys[27].constantID = 27;
        uberSpecializationMapEntrys[27].offset = sizeof(u32) * 3 + sizeof(f32) * 24;
        uberSpecializationMapEntrys[27].size = sizeof(f32);

        uberSpecializationMapEntrys[28].constantID = 28;
        uberSpecializationMapEntrys[28].offset = sizeof(u32) * 3 + sizeof(f32) * 25;
        uberSpecializationMapEntrys[28].size = sizeof(u32);
        uberSpecializationMapEntrys[29].constantID = 29;
        uberSpecializationMapEntrys[29].offset = sizeof(u32) * 4 + sizeof(f32) * 25;
        uberSpecializationMapEntrys[29].size = sizeof(f32);
        uberSpecializationMapEntrys[30].constantID = 30;
        uberSpecializationMapEntrys[30].offset = sizeof(u32) * 4 + sizeof(f32) * 26;
        uberSpecializationMapEntrys[30].size = sizeof(f32);
        uberSpecializationMapEntrys[31].constantID = 31;
        uberSpecializationMapEntrys[31].offset = sizeof(u32) * 4 + sizeof(f32) * 27;
        uberSpecializationMapEntrys[31].size = sizeof(f32);

        uberSpecializationMapEntrys[32].constantID = 32;
        uberSpecializationMapEntrys[32].offset = sizeof(u32) * 4 + sizeof(f32) * 28;
        uberSpecializationMapEntrys[32].size = sizeof(u32);
        uberSpecializationMapEntrys[33].constantID = 33;
        uberSpecializationMapEntrys[33].offset = sizeof(u32) * 5 + sizeof(f32) * 28;
        uberSpecializationMapEntrys[33].size = sizeof(f32);

        struct {
            u32 samples;
            f32 radius;
            f32 multiplier;
            f32 scale;
            f32 bias;
            f32 maxDistance;
            f32 goldenAngle;
        } ssaoSpecializationData = {
            config.ssaoSamples, config.ssaoRadius, config.ssaoMultiplier, config.ssaoScale,
            config.ssaoBias, config.ssaoMaxDistance, config.ssaoGoldenAngle
        };

        struct {
            i32 ssaoDenoiseSize;
            f32 ssaoDenoiseExponent;
            f32 ssaoDenoiseFactor;

            f32 ambientLightColorR;
            f32 ambientLightColorG;
            f32 ambientLightColorB;
            f32 ambientLightIntensity;
            f32 directionalLightColorR;
            f32 directionalLightColorG;
            f32 directionalLightColorB;
            f32 directionalLightIntensity;
            f32 volumetricLightScatteringFactor;
            u32 volumetricLightSteps;

            f32 atmosphereSunPower;
            u32 atmosphereSteps;
            u32 atmosphereLightSteps;
        } compositionSpecializationData = {
            config.ssaoDenoiseSize, config.ssaoDenoiseExponent, config.ssaoDenoiseFactor,
            config.ambientLightColor[0], config.ambientLightColor[1], config.ambientLightColor[2], config.ambientLightIntensity,
            config.directionalLightColor[0], config.directionalLightColor[1], config.directionalLightColor[2], config.directionalLightIntensity,
            config.volumetricLightScatteringFactor, config.volumetricLightSteps,
            config.atmosphereSunPower, config.atmosphereSteps, config.atmosphereLightSteps
        };

        struct {
            f32 bloomIntensity;
            f32 bloomThreshold;
        } gaussianBlurSpecializationData = {config.bloomIntensity, config.bloomThreshold};

        struct {
            f32 targetFps;

            f32 gammaR;
            f32 gammaG;
            f32 gammaB;
            f32 exposureR;
            f32 exposureG;
            f32 exposureB;
            f32 agxLookOffsetR;
            f32 agxLookOffsetG;
            f32 agxLookOffsetB;
            f32 agxLookSlopeR;
            f32 agxLookSlopeG;
            f32 agxLookSlopeB;
            f32 agxLookPowerR;
            f32 agxLookPowerG;
            f32 agxLookPowerB;
            f32 agxLookSaturationR;
            f32 agxLookSaturationG;
            f32 agxLookSaturationB;

            u32 grainEnable;
            f32 grainIntensity;
            f32 grainSignalToNoiseRatio;
            f32 grainNoiseShift;

            u32 ditheringEnable;
            f32 ditheringToneCount;

            u32 motionBlurEnable;
            u32 motionBlurMaxSamples;
            f32 motionBlurVelocityReductionFactor;

            u32 fxaaEnable;
            f32 fxaaReduceMin;
            f32 fxaaReduceMul;
            f32 fxaaSpanMax;

            u32 bloomEnable;
            f32 bloomIntensity;
        } uberSpecializationData = {
            config.targetFps,
            config.gamma[0], config.gamma[1], config.gamma[2],
            config.exposure[0], config.exposure[1], config.exposure[2],
            config.agxLookOffset[0], config.agxLookOffset[1], config.agxLookOffset[2],
            config.agxLookSlope[0], config.agxLookSlope[1], config.agxLookSlope[2],
            config.agxLookPower[0], config.agxLookPower[1], config.agxLookPower[2],
            config.agxLookSaturation[0], config.agxLookSaturation[1], config.agxLookSaturation[2],
            config.grainEnable, config.grainIntensity, config.grainSignalToNoise, config.grainNoiseShift,
            config.ditheringEnable, config.ditheringToneCount,
            config.motionBlurEnable, config.motionBlurMaxSamples, config.motionBlurVelocityReductionFactor,
            config.fxaaEnable, config.fxaaReduceMin, config.fxaaReduceMul, config.fxaaSpanMax,
            config.bloomEnable, config.bloomIntensity
        };

        VkSpecializationInfo specializationInfos[4] = {};
        specializationInfos[0].mapEntryCount = 7;
        specializationInfos[0].pMapEntries = ssaoSpecializationMapEntrys;
        specializationInfos[0].dataSize = sizeof(u32) + sizeof(f32) * 6;
        specializationInfos[0].pData = &ssaoSpecializationData;

        specializationInfos[1].mapEntryCount = 16;
        specializationInfos[1].pMapEntries = compositionSpecializationMapEntrys;
        specializationInfos[1].dataSize = sizeof(i32) + sizeof(u32) * 3 + sizeof(f32) * 12;
        specializationInfos[1].pData = &compositionSpecializationData;

        specializationInfos[2].mapEntryCount = 2;
        specializationInfos[2].pMapEntries = gaussianBlurSpecializationMapEntrys;
        specializationInfos[2].dataSize = sizeof(f32) * 2;
        specializationInfos[2].pData = &gaussianBlurSpecializationData;

        specializationInfos[3].mapEntryCount = 34;
        specializationInfos[3].pMapEntries = uberSpecializationMapEntrys;
        specializationInfos[3].dataSize = sizeof(u32) * 5 + sizeof(f32) * 29;
        specializationInfos[3].pData = &uberSpecializationData;

        VkVertexInputBindingDescription bindingDescs[2] = {};
        bindingDescs[0].binding = 0;
        bindingDescs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDescs[0].stride = sizeof(vec3) * 3 + sizeof(vec2);

        bindingDescs[1].binding = 0;
        bindingDescs[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDescs[1].stride = sizeof(vec3);

        VkVertexInputAttributeDescription attributeDescs[4] = {};
        attributeDescs[0].binding = 0;
        attributeDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescs[0].location = 0;
        attributeDescs[0].offset = 0;

        attributeDescs[1].binding = 0;
        attributeDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescs[1].location = 1;
        attributeDescs[1].offset = sizeof(vec3);

        attributeDescs[2].binding = 0;
        attributeDescs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescs[2].location = 2;
        attributeDescs[2].offset = sizeof(vec3) * 2;

        attributeDescs[3].binding = 0;
        attributeDescs[3].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescs[3].location = 3;
        attributeDescs[3].offset = sizeof(vec3) * 3;

        VkShaderModule fullscreenVertexModule = createShaderModuleFromAsset("assets/shaders/fullscreen.vert.spv");
        VkShaderModule fullscreenNoUVVertexModule = createShaderModuleFromAsset("assets/shaders/fullscreenNoUV.vert.spv");

        VkGraphicsPipelineInfo_t pipelineInfos[7] = {};
        pipelineFillDefaultGraphicsPipeline(&pipelineInfos[0]);
        pipelineFillDefaultGraphicsPipeline(&pipelineInfos[1]);
        pipelineFillDefaultGraphicsPipeline(&pipelineInfos[2]);
        pipelineFillDefaultGraphicsPipeline(&pipelineInfos[3]);
        pipelineFillDefaultGraphicsPipeline(&pipelineInfos[4]);
        pipelineFillDefaultGraphicsPipeline(&pipelineInfos[5]);
        pipelineFillDefaultGraphicsPipeline(&pipelineInfos[6]);


        pipelineInfos[0].stageCount = 1;
        pipelineInfos[0].stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        pipelineInfos[0].stages[0].module = createShaderModuleFromAsset("assets/shaders/shadowmap.vert.spv");

        pipelineInfos[0].vertexInputState.vertexAttributeDescriptionCount = 1;
        pipelineInfos[0].vertexInputState.pVertexAttributeDescriptions = attributeDescs;
        pipelineInfos[0].vertexInputState.vertexBindingDescriptionCount = 1;
        pipelineInfos[0].vertexInputState.pVertexBindingDescriptions = bindingDescs;
        pipelineInfos[0].depthStencilState.depthTestEnable = VK_TRUE;
        pipelineInfos[0].depthStencilState.depthWriteEnable = VK_TRUE;

        pipelineInfos[0].rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
        pipelineInfos[0].rasterizationState.depthBiasEnable = VK_TRUE;
        pipelineInfos[0].rasterizationState.depthBiasSlopeFactor = -1.0;
        pipelineInfos[0].rasterizationState.depthBiasConstantFactor = 0.0;
        pipelineInfos[0].rasterizationState.depthBiasClamp = 0.0;

        pipelineInfos[0].viewport.width *= SHADOWMAP_RESOLUTION;
        pipelineInfos[0].viewport.height *= SHADOWMAP_RESOLUTION;
        pipelineInfos[0].scissor.extent.width *= SHADOWMAP_RESOLUTION;
        pipelineInfos[0].scissor.extent.height *= SHADOWMAP_RESOLUTION;

        pipelineInfos[0].renderingInfo.depthAttachmentFormat = globals->depthFormat;
        pipelineInfos[0].layout = globals->shadowmapPipelineLayout;


        pipelineInfos[1].stageCount = 2;
        pipelineInfos[1].stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        pipelineInfos[1].stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineInfos[1].stages[0].module = createShaderModuleFromAsset("assets/shaders/model.vert.spv");
        pipelineInfos[1].stages[1].module = createShaderModuleFromAsset("assets/shaders/model.frag.spv");

        pipelineInfos[1].vertexInputState.vertexAttributeDescriptionCount = 4;
        pipelineInfos[1].vertexInputState.pVertexAttributeDescriptions = attributeDescs;
        pipelineInfos[1].vertexInputState.vertexBindingDescriptionCount = 1;
        pipelineInfos[1].vertexInputState.pVertexBindingDescriptions = bindingDescs;
        pipelineInfos[1].depthStencilState.depthTestEnable = VK_TRUE;
        pipelineInfos[1].depthStencilState.depthWriteEnable = VK_TRUE;
        pipelineInfos[1].colorBlendState.attachmentCount = 3;
        pipelineInfos[1].colorBlendState.pAttachments = (VkPipelineColorBlendAttachmentState[]){pipelineInfos[1].colorBlendAttachment, pipelineInfos[1].colorBlendAttachment, pipelineInfos[1].colorBlendAttachment};

        if (config.wireframe) {
            pipelineInfos[1].rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
            // since triangles are transparent in wireframe mode, we disable culling
            pipelineInfos[1].rasterizationState.cullMode = VK_CULL_MODE_NONE;
        }

        pipelineInfos[1].renderingInfo.colorAttachmentCount = 3;
        pipelineInfos[1].renderingInfo.pColorAttachmentFormats = (VkFormat[]){VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_SFLOAT};
        pipelineInfos[1].renderingInfo.depthAttachmentFormat = globals->depthFormat;
        pipelineInfos[1].layout = globals->modelPipelineLayout;


        pipelineInfos[2].stageCount = 2;
        pipelineInfos[2].stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        pipelineInfos[2].stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineInfos[2].stages[0].module = createShaderModuleFromAsset("assets/shaders/skybox.vert.spv");
        pipelineInfos[2].stages[1].module = createShaderModuleFromAsset("assets/shaders/skybox.frag.spv");

        pipelineInfos[2].vertexInputState.vertexAttributeDescriptionCount = 1;
        pipelineInfos[2].vertexInputState.pVertexAttributeDescriptions = attributeDescs;
        pipelineInfos[2].vertexInputState.vertexBindingDescriptionCount = 1;
        pipelineInfos[2].vertexInputState.pVertexBindingDescriptions = bindingDescs + 1;
        //pipelineInfos[2].rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        pipelineInfos[2].depthStencilState.depthTestEnable = VK_TRUE;
        pipelineInfos[2].colorBlendState.attachmentCount = 3;
        pipelineInfos[2].colorBlendState.pAttachments = (VkPipelineColorBlendAttachmentState[]){pipelineInfos[0].colorBlendAttachment, pipelineInfos[0].colorBlendAttachment, pipelineInfos[0].colorBlendAttachment};

        pipelineInfos[2].renderingInfo.colorAttachmentCount = 3;
        pipelineInfos[2].renderingInfo.pColorAttachmentFormats = (VkFormat[]){VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_SFLOAT};
        pipelineInfos[2].renderingInfo.depthAttachmentFormat = globals->depthFormat;
        pipelineInfos[2].layout = globals->PVPipelineLayout;


        pipelineInfos[3].stageCount = 2;
        pipelineInfos[3].stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        pipelineInfos[3].stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineInfos[3].stages[0].module = fullscreenVertexModule;
        pipelineInfos[3].stages[1].module = createShaderModuleFromAsset("assets/shaders/ssao.frag.spv");
        pipelineInfos[3].stages[1].pSpecializationInfo = &specializationInfos[0];

        pipelineInfos[3].viewport.width = config.ssaoResolutionWidth;
        pipelineInfos[3].viewport.height = config.ssaoResolutionHeight;

        pipelineInfos[3].renderingInfo.colorAttachmentCount = 1;
        pipelineInfos[3].renderingInfo.pColorAttachmentFormats = (VkFormat[]){VK_FORMAT_R8_UNORM};
        pipelineInfos[3].layout = globals->PVgbufferPipelineLayout;


        pipelineInfos[4].stageCount = 2;
        pipelineInfos[4].stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        pipelineInfos[4].stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineInfos[4].stages[0].module = fullscreenVertexModule;
        pipelineInfos[4].stages[1].module = createShaderModuleFromAsset("assets/shaders/composition.frag.spv");
        pipelineInfos[4].stages[1].pSpecializationInfo = &specializationInfos[1];

        pipelineInfos[4].renderingInfo.colorAttachmentCount = 1;
        pipelineInfos[4].renderingInfo.pColorAttachmentFormats = (VkFormat[]){VK_FORMAT_R16G16B16A16_SFLOAT};
        pipelineInfos[4].layout = globals->compositionPipelineLayout;


        pipelineInfos[5].stageCount = 2;
        pipelineInfos[5].stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        pipelineInfos[5].stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineInfos[5].stages[0].module = fullscreenVertexModule;
        pipelineInfos[5].stages[1].module = createShaderModuleFromAsset("assets/shaders/postprocess/uber.frag.spv");
        pipelineInfos[5].stages[1].pSpecializationInfo = &specializationInfos[3];

        pipelineInfos[5].renderingInfo.colorAttachmentCount = 1;
        pipelineInfos[5].renderingInfo.pColorAttachmentFormats = &vkglobals.surfaceFormat.format;
        pipelineInfos[5].layout = globals->uberPipelineLayout;

        pipelineInfos[6].stageCount = 2;
        pipelineInfos[6].stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        pipelineInfos[6].stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineInfos[6].stages[0].module = fullscreenNoUVVertexModule;
        pipelineInfos[6].stages[1].module = createShaderModuleFromAsset("assets/shaders/gaussianBlur.frag.spv");
        pipelineInfos[6].stages[1].pSpecializationInfo = &specializationInfos[2];

        pipelineInfos[6].renderingInfo.colorAttachmentCount = 1;
        pipelineInfos[6].renderingInfo.pColorAttachmentFormats = (VkFormat[]){VK_FORMAT_R16G16B16A16_SFLOAT};
        pipelineInfos[6].layout = globals->sampledImagePipelineLayout;

        VkPipeline pipelines[7];

        pipelineCreateGraphicsPipelines(globals->pipelineCache, 7, pipelineInfos, pipelines);

        globals->shadowmapPipeline = pipelines[0];
        globals->modelPipeline = pipelines[1];
        globals->skyboxPipeline = pipelines[2];
        globals->ssaoPipeline = pipelines[3];
        globals->compositionPipeline = pipelines[4];
        globals->uberPipeline = pipelines[5];
        globals->gaussianBlurPipeline = pipelines[6];

        vkDestroyShaderModule(vkglobals.device, pipelineInfos[6].stages[1].module, VK_NULL_HANDLE);
        vkDestroyShaderModule(vkglobals.device, pipelineInfos[5].stages[1].module, VK_NULL_HANDLE);
        vkDestroyShaderModule(vkglobals.device, pipelineInfos[4].stages[1].module, VK_NULL_HANDLE);
        vkDestroyShaderModule(vkglobals.device, pipelineInfos[3].stages[1].module, VK_NULL_HANDLE);
        vkDestroyShaderModule(vkglobals.device, pipelineInfos[2].stages[0].module, VK_NULL_HANDLE);
        vkDestroyShaderModule(vkglobals.device, pipelineInfos[2].stages[1].module, VK_NULL_HANDLE);
        vkDestroyShaderModule(vkglobals.device, pipelineInfos[1].stages[0].module, VK_NULL_HANDLE);
        vkDestroyShaderModule(vkglobals.device, pipelineInfos[1].stages[1].module, VK_NULL_HANDLE);
        vkDestroyShaderModule(vkglobals.device, pipelineInfos[0].stages[0].module, VK_NULL_HANDLE);
        vkDestroyShaderModule(vkglobals.device, fullscreenNoUVVertexModule, VK_NULL_HANDLE);
        vkDestroyShaderModule(vkglobals.device, fullscreenVertexModule, VK_NULL_HANDLE);
    }

    {
        globals->renderingDoneSemaphores = (VkSemaphore*)malloc(sizeof(VkSemaphore) * globals->swapchainImageCount);

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        for (u32 i = 0; i < globals->swapchainImageCount; i++) VK_ASSERT(vkCreateSemaphore(vkglobals.device, &semaphoreInfo, VK_NULL_HANDLE, &globals->renderingDoneSemaphores[i]), "failed to create semaphore\n");
        VK_ASSERT(vkCreateSemaphore(vkglobals.device, &semaphoreInfo, VK_NULL_HANDLE, &globals->swapchainReadySemaphore), "failed to create semaphore\n");

        VkFenceCreateInfo fenceInfos[1] = {};
        fenceInfos[0].sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfos[0].flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VK_ASSERT(vkCreateFence(vkglobals.device, &fenceInfos[0], VK_NULL_HANDLE, &globals->frameFence), "failed to create fence\n");
    }

    globals->shift = 1;

    globals->cam.position[0] = 0.0f;
    globals->cam.position[1] = 0.0f;
    globals->cam.position[2] = 0.0f;

    globals->cam.yaw = 0.0f;
    globals->cam.pitch = 0.0f;
    globals->cam.targetYaw = 0.0f;
    globals->cam.targetPitch = 0.0f;

    globals->inputX[0] = 0.0f;
    globals->inputX[1] = 0.0f;
    globals->inputY[0] = 0.0f;
    globals->inputY[1] = 0.0f;
    globals->inputZ[0] = 0.0f;
    globals->inputZ[1] = 0.0f;

    glm_mat4_identity(globals->hostVisibleUniformMemoryRaw);

    globals->lightPos[0] = -3.317797f;
    globals->lightPos[1] = -22.590681f;
    globals->lightPos[2] = -6.158334f;
    globals->lightPos[3] = 1.0f;

    tempResourcesWaitAndDestroy(1, &tempCmdBuffer, 1, &tempBuffer, 1, &tempBufferMemory, 1, &tempFence);
}

void modelViewSceneEvent(SDL_Event* e) {
    if (e->type == SDL_EVENT_KEY_DOWN) {
        if (e->key.key == SDLK_W) globals->inputZ[0] = -config.playerSpeed / 1e9f;
        else if (e->key.key == SDLK_S) globals->inputZ[1] = config.playerSpeed / 1e9f;
        else if (e->key.key == SDLK_A) globals->inputX[0] = -config.playerSpeed / 1e9f;
        else if (e->key.key == SDLK_D) globals->inputX[1] = config.playerSpeed / 1e9f;
        else if (e->key.key == SDLK_SPACE) globals->inputY[0] = -config.playerSpeed / 1e9f;
        else if (e->key.key == SDLK_LCTRL) globals->inputY[1] = config.playerSpeed / 1e9f;
        else if (e->key.key == SDLK_LSHIFT) globals->shift = config.shiftMultiplier;
        else if (e->key.key == SDLK_ESCAPE) vkglobals.loopActive = 0;
    } else if (e->type == SDL_EVENT_KEY_UP) {
        if (e->key.key == SDLK_W) globals->inputZ[0] = 0;
        else if (e->key.key == SDLK_S) globals->inputZ[1] = 0;
        else if (e->key.key == SDLK_A) globals->inputX[0] = 0;
        else if (e->key.key == SDLK_D) globals->inputX[1] = 0;
        else if (e->key.key == SDLK_SPACE) globals->inputY[0] = 0;
        else if (e->key.key == SDLK_LCTRL) globals->inputY[1] = 0;
        else if (e->key.key == SDLK_LSHIFT) globals->shift = 1;
    } else if (e->type == SDL_EVENT_MOUSE_MOTION) {
        globals->cam.targetYaw += (f32)e->motion.xrel / 400.0f;
        globals->cam.targetPitch += (f32)e->motion.yrel / 400.0f;
        clampf(&globals->cam.targetPitch, -M_PI / 2.0f, M_PI / 2.0f);
    }
}

void updateCubeUbo() {
    #define modelMatrix globals->hostVisibleStorageMemoryRaw
    #define viewMatrix globals->hostVisibleUniformMemoryRaw

    if (config.mouseSmoothingEnable) {
        // this one seems fine, but maybe move to something more sophisticated
        globals->cam.yaw += (globals->cam.targetYaw - globals->cam.yaw) * (1.0f - expf((vkglobals.deltaTime / 1e9f) * -config.mouseSmoothingSpeed));
        globals->cam.pitch += (globals->cam.targetPitch - globals->cam.pitch) * (1.0f - expf((vkglobals.deltaTime / 1e9f) * -config.mouseSmoothingSpeed));
    } else {
        globals->cam.yaw = globals->cam.targetYaw;
        globals->cam.pitch = globals->cam.targetPitch;
    }

    versor y, p;
    glm_quatv(y, globals->cam.yaw, (vec3){0.0f, -1.0f, 0.0f});
    glm_quatv(p, globals->cam.pitch, (vec3){1.0f, 0.0f, 0.0f});
    
    {
        mat4 rot;
        glm_quat_mat4(y, rot);
        vec3 vel;
        glm_mat4_mulv3(rot, (vec3){(globals->inputX[0] + globals->inputX[1]) * globals->shift * vkglobals.deltaTime, 0.0f, (globals->inputZ[0] + globals->inputZ[1]) * globals->shift * vkglobals.deltaTime}, 0.0f, vel);
        vel[1] += (globals->inputY[0] + globals->inputY[1]) * globals->shift * vkglobals.deltaTime;
        glm_vec3_add(globals->cam.position, vel, globals->cam.position);
    }

    {
        glm_quat_mul(y, p, y);

        glm_mat4_copy(viewMatrix, viewMatrix + sizeof(mat4) * 2);
        glm_quat_look(globals->cam.position, y, viewMatrix);
        glm_mat4_inv(viewMatrix, viewMatrix + sizeof(mat4));
    }

    glm_mat4_identity(modelMatrix);
    glm_spin(modelMatrix, M_PI, (vec3){0.0f, 0.0f, 1.0f});

    vec4 viewLightPos;
    glm_mat4_mulv(viewMatrix, globals->lightPos, viewLightPos);

    mat4 proj, view;

    glm_ortho(-18.0f, 18.0f, 18.0f, -18.0f, 30.0f, 0.01f, proj);
    glm_lookat(globals->lightPos, (vec3){0.0f, 0.0f, 0.0f}, (vec3){0.0f, 1.0f, 0.0f}, view);
    glm_mat4_mul(proj, view, proj);

    memcpy(globals->hostVisibleStorageMemoryRaw + globals->lightBufferOffset, proj, sizeof(mat4));
    memcpy(globals->hostVisibleStorageMemoryRaw + globals->lightBufferOffset + sizeof(mat4), &viewLightPos, sizeof(vec4));

    VkMappedMemoryRange memoryRanges[2] = {};
    memoryRanges[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memoryRanges[0].offset = 0;
    memoryRanges[0].size = VK_WHOLE_SIZE;
    memoryRanges[0].memory = globals->hostVisibleUniformMemory;

    memoryRanges[1].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memoryRanges[1].offset = 0;
    memoryRanges[1].size = VK_WHOLE_SIZE;
    memoryRanges[1].memory = globals->hostVisibleStorageMemory;

    VK_ASSERT(vkFlushMappedMemoryRanges(vkglobals.device, 2, memoryRanges), "failed to flush device memory\n");
}

void modelViewSceneRender() {
    VK_ASSERT(vkWaitForFences(vkglobals.device, 1, &globals->frameFence, VK_TRUE, 0xFFFFFFFFFFFFFFFF), "failed to wait for fences\n");
    VK_ASSERT(vkResetFences(vkglobals.device, 1, &globals->frameFence), "failed to reset fences\n");

    u32 imageIndex;
    VK_ASSERT(vkAcquireNextImageKHR(vkglobals.device, vkglobals.swapchain, 0xFFFFFFFFFFFFFFFF, globals->swapchainReadySemaphore, VK_NULL_HANDLE, &imageIndex), "failed to acquire swapchain image\n");

    updateCubeUbo();

    {
        VkCommandBufferBeginInfo cmdBeginInfo = {};
        cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_ASSERT(vkBeginCommandBuffer(globals->cmdBuffer, &cmdBeginInfo), "failed to begin command buffer\n");

        {
            VkImageMemoryBarrier imageBarriers[1] = {};
            imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarriers[0].image = globals->shadowmap;
            imageBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            imageBarriers[0].subresourceRange.baseArrayLayer = 0;
            imageBarriers[0].subresourceRange.layerCount = 1;
            imageBarriers[0].subresourceRange.baseMipLevel = 0;
            imageBarriers[0].subresourceRange.levelCount = 1;
            imageBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[0].srcAccessMask = 0;
            imageBarriers[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            vkCmdPipelineBarrier(globals->cmdBuffer, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, imageBarriers);
        }

        {
            VkRenderingAttachmentInfoKHR attachments[1] = {};
            attachments[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            attachments[0].imageView = globals->shadowmapView;
            attachments[0].imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachments[0].resolveMode = VK_RESOLVE_MODE_NONE_KHR;
            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[0].clearValue = (VkClearValue){{{0.0f, 0}}};

            VkRenderingInfoKHR renderingInfo = {};
            renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
            renderingInfo.renderArea = (VkRect2D){(VkOffset2D){0, 0}, (VkExtent2D){vkglobals.swapchainExtent.width * SHADOWMAP_RESOLUTION, vkglobals.swapchainExtent.height * SHADOWMAP_RESOLUTION}};
            renderingInfo.layerCount = 1;
            renderingInfo.colorAttachmentCount = 0;
            renderingInfo.pColorAttachments = VK_NULL_HANDLE;
            renderingInfo.pDepthAttachment = attachments;
            renderingInfo.pStencilAttachment = VK_NULL_HANDLE;

            vkCmdBeginRenderingKHR(globals->cmdBuffer, &renderingInfo);

            VkDeviceSize vertexBufferOffsets[1] = {};
            vkCmdBindPipeline(globals->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, globals->shadowmapPipeline);
            vkCmdBindVertexBuffers(globals->cmdBuffer, 0, 1, &globals->model.vertexBuffer, vertexBufferOffsets);
            vkCmdBindIndexBuffer(globals->cmdBuffer, globals->model.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdBindDescriptorSets(globals->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, globals->shadowmapPipelineLayout, 0, 2, (VkDescriptorSet[]){globals->lightDescriptorSet, globals->model.descriptorSet}, 0, VK_NULL_HANDLE);

            vkCmdDrawIndexedIndirect(globals->cmdBuffer, globals->model.indirectBuffer, 0, globals->model.drawCount, sizeof(VkDrawIndexedIndirectCommand));

            vkCmdEndRenderingKHR(globals->cmdBuffer);
        }

        {
            VkImageMemoryBarrier imageBarriers[3] = {};
            imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarriers[0].image = globals->gbuffer;
            imageBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarriers[0].subresourceRange.baseArrayLayer = 0;
            imageBarriers[0].subresourceRange.layerCount = 2;
            imageBarriers[0].subresourceRange.baseMipLevel = 0;
            imageBarriers[0].subresourceRange.levelCount = 1;
            imageBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[0].srcAccessMask = 0;
            imageBarriers[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            imageBarriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarriers[1].image = globals->metallicRoughnessVelocityTexture;
            imageBarriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarriers[1].subresourceRange.baseArrayLayer = 0;
            imageBarriers[1].subresourceRange.layerCount = 1;
            imageBarriers[1].subresourceRange.baseMipLevel = 0;
            imageBarriers[1].subresourceRange.levelCount = 1;
            imageBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[1].srcAccessMask = 0;
            imageBarriers[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            imageBarriers[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageBarriers[1].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            imageBarriers[2].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarriers[2].image = globals->depthTexture;
            imageBarriers[2].subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            imageBarriers[2].subresourceRange.baseArrayLayer = 0;
            imageBarriers[2].subresourceRange.layerCount = 1;
            imageBarriers[2].subresourceRange.baseMipLevel = 0;
            imageBarriers[2].subresourceRange.levelCount = 1;
            imageBarriers[2].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[2].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[2].srcAccessMask = 0;
            imageBarriers[2].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            imageBarriers[2].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageBarriers[2].newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            vkCmdPipelineBarrier(globals->cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 2, imageBarriers);
            vkCmdPipelineBarrier(globals->cmdBuffer, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, imageBarriers + 2);
        }

        {
            VkRenderingAttachmentInfoKHR attachments[4] = {};
            attachments[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            attachments[0].imageView = globals->gbufferNormalView;
            attachments[0].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachments[0].resolveMode = VK_RESOLVE_MODE_NONE_KHR;
            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[0].clearValue = (VkClearValue){{{0.0f, 0.0f, 0.0f, 0.0f}}};

            attachments[1].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            attachments[1].imageView = globals->gbufferAlbedoView;
            attachments[1].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachments[1].resolveMode = VK_RESOLVE_MODE_NONE_KHR;
            attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[1].clearValue = (VkClearValue){{{0.0f, 0.0f, 0.0f, 0.0f}}};

            attachments[2].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            attachments[2].imageView = globals->metallicRoughnessVelocityTextureView;
            attachments[2].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachments[2].resolveMode = VK_RESOLVE_MODE_NONE_KHR;
            attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[2].clearValue = (VkClearValue){{{0.0f, 0.0f, 0.0f, 0.0f}}};

            attachments[3].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            attachments[3].imageView = globals->depthTextureView;
            attachments[3].imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachments[3].resolveMode = VK_RESOLVE_MODE_NONE_KHR;
            attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[3].clearValue = (VkClearValue){{{0.0f, 0}}};

            VkRenderingInfoKHR renderingInfo = {};
            renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
            renderingInfo.renderArea = (VkRect2D){(VkOffset2D){0, 0}, vkglobals.swapchainExtent};
            renderingInfo.layerCount = 1;
            renderingInfo.colorAttachmentCount = 3;
            renderingInfo.pColorAttachments = attachments;
            renderingInfo.pDepthAttachment = attachments + 3;
            renderingInfo.pStencilAttachment = VK_NULL_HANDLE;

            vkCmdBeginRenderingKHR(globals->cmdBuffer, &renderingInfo);

            VkDeviceSize vertexBufferOffsets[1] = {};
            vkCmdBindPipeline(globals->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, globals->modelPipeline);
            vkCmdBindVertexBuffers(globals->cmdBuffer, 0, 1, &globals->model.vertexBuffer, vertexBufferOffsets);
            vkCmdBindIndexBuffer(globals->cmdBuffer, globals->model.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdBindDescriptorSets(globals->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, globals->modelPipelineLayout, 0, 2, (VkDescriptorSet[]){globals->PVmatrixdescriptorSet, globals->model.descriptorSet}, 0, VK_NULL_HANDLE);

            vkCmdDrawIndexedIndirect(globals->cmdBuffer, globals->model.indirectBuffer, 0, globals->model.drawCount, sizeof(VkDrawIndexedIndirectCommand));

            vkCmdBindPipeline(globals->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, globals->skyboxPipeline);
            vkCmdBindVertexBuffers(globals->cmdBuffer, 0, 1, &globals->skyboxVertexBuffer, vertexBufferOffsets);
            vkCmdBindDescriptorSets(globals->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, globals->PVPipelineLayout, 0, 1, (VkDescriptorSet[]){globals->PVmatrixdescriptorSet}, 0, VK_NULL_HANDLE);

            vkCmdDraw(globals->cmdBuffer, 36, 1, 0, 0);

            vkCmdEndRenderingKHR(globals->cmdBuffer);
        }

        {
            VkImageMemoryBarrier imageBarriers[4] = {};
            imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarriers[0].image = globals->ssaoAttachment;
            imageBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarriers[0].subresourceRange.baseArrayLayer = 0;
            imageBarriers[0].subresourceRange.layerCount = 1;
            imageBarriers[0].subresourceRange.baseMipLevel = 0;
            imageBarriers[0].subresourceRange.levelCount = 1;
            imageBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[0].srcAccessMask = 0;
            imageBarriers[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            imageBarriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarriers[1].image = globals->gbuffer;
            imageBarriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarriers[1].subresourceRange.baseArrayLayer = 0;
            imageBarriers[1].subresourceRange.layerCount = 2;
            imageBarriers[1].subresourceRange.baseMipLevel = 0;
            imageBarriers[1].subresourceRange.levelCount = 1;
            imageBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            imageBarriers[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            imageBarriers[1].oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            imageBarriers[1].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            imageBarriers[2].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarriers[2].image = globals->metallicRoughnessVelocityTexture;
            imageBarriers[2].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarriers[2].subresourceRange.baseArrayLayer = 0;
            imageBarriers[2].subresourceRange.layerCount = 1;
            imageBarriers[2].subresourceRange.baseMipLevel = 0;
            imageBarriers[2].subresourceRange.levelCount = 1;
            imageBarriers[2].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[2].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            imageBarriers[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            imageBarriers[2].oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            imageBarriers[2].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            imageBarriers[3].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarriers[3].image = globals->depthTexture;
            imageBarriers[3].subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            imageBarriers[3].subresourceRange.baseArrayLayer = 0;
            imageBarriers[3].subresourceRange.layerCount = 1;
            imageBarriers[3].subresourceRange.baseMipLevel = 0;
            imageBarriers[3].subresourceRange.levelCount = 1;
            imageBarriers[3].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[3].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[3].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            imageBarriers[3].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            imageBarriers[3].oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            imageBarriers[3].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
           
            vkCmdPipelineBarrier(globals->cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, imageBarriers);
            vkCmdPipelineBarrier(globals->cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 2, imageBarriers + 1);
            vkCmdPipelineBarrier(globals->cmdBuffer, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, imageBarriers + 3);
        }

        {
            VkRenderingAttachmentInfoKHR attachments[1] = {};
            attachments[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            attachments[0].imageView = globals->ssaoAttachmentView;
            attachments[0].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachments[0].resolveMode = VK_RESOLVE_MODE_NONE_KHR;
            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[0].clearValue = (VkClearValue){{{0.0f, 0.0f, 0.0f, 0.0f}}};

            VkRenderingInfoKHR renderingInfo = {};
            renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
            renderingInfo.renderArea = (VkRect2D){(VkOffset2D){0, 0}, (VkExtent2D){config.ssaoResolutionWidth, config.ssaoResolutionHeight}};
            renderingInfo.layerCount = 1;
            renderingInfo.colorAttachmentCount = 1;
            renderingInfo.pColorAttachments = attachments;
            renderingInfo.pDepthAttachment = VK_NULL_HANDLE;
            renderingInfo.pStencilAttachment = VK_NULL_HANDLE;

            vkCmdBeginRenderingKHR(globals->cmdBuffer, &renderingInfo);

            vkCmdBindPipeline(globals->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, globals->ssaoPipeline);
            vkCmdBindDescriptorSets(globals->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, globals->PVgbufferPipelineLayout, 0, 2, (VkDescriptorSet[]){globals->PVmatrixdescriptorSet, globals->gbufferDescriptorSet}, 0, VK_NULL_HANDLE);

            vkCmdDraw(globals->cmdBuffer, 3, 1, 0, 0);

            vkCmdEndRenderingKHR(globals->cmdBuffer);
        }

        {
            VkImageMemoryBarrier imageBarriers[3] = {};
            imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarriers[0].image = globals->postProcessAttachment;
            imageBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarriers[0].subresourceRange.baseArrayLayer = 0;
            imageBarriers[0].subresourceRange.layerCount = 1;
            imageBarriers[0].subresourceRange.baseMipLevel = 0;
            imageBarriers[0].subresourceRange.levelCount = 1;
            imageBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[0].srcAccessMask = 0;
            imageBarriers[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            imageBarriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarriers[1].image = globals->ssaoAttachment;
            imageBarriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarriers[1].subresourceRange.baseArrayLayer = 0;
            imageBarriers[1].subresourceRange.layerCount = 1;
            imageBarriers[1].subresourceRange.baseMipLevel = 0;
            imageBarriers[1].subresourceRange.levelCount = 1;
            imageBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            imageBarriers[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            imageBarriers[1].oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            imageBarriers[1].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            imageBarriers[2].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarriers[2].image = globals->shadowmap;
            imageBarriers[2].subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            imageBarriers[2].subresourceRange.baseArrayLayer = 0;
            imageBarriers[2].subresourceRange.layerCount = 1;
            imageBarriers[2].subresourceRange.baseMipLevel = 0;
            imageBarriers[2].subresourceRange.levelCount = 1;
            imageBarriers[2].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[2].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[2].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            imageBarriers[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            imageBarriers[2].oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            imageBarriers[2].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            vkCmdPipelineBarrier(globals->cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, imageBarriers);
            vkCmdPipelineBarrier(globals->cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, imageBarriers + 1);
            vkCmdPipelineBarrier(globals->cmdBuffer, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, imageBarriers + 2);
        }

        {
            VkRenderingAttachmentInfoKHR attachments[1] = {};
            attachments[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            attachments[0].imageView = globals->postProcessAttachmentView;
            attachments[0].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachments[0].resolveMode = VK_RESOLVE_MODE_NONE_KHR;
            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[0].clearValue = (VkClearValue){{{0.0f, 0.0f, 0.0f, 0.0f}}};

            VkRenderingInfoKHR renderingInfo = {};
            renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
            renderingInfo.renderArea = (VkRect2D){(VkOffset2D){0, 0}, vkglobals.swapchainExtent};
            renderingInfo.layerCount = 1;
            renderingInfo.colorAttachmentCount = 1;
            renderingInfo.pColorAttachments = attachments;
            renderingInfo.pDepthAttachment = VK_NULL_HANDLE;
            renderingInfo.pStencilAttachment = VK_NULL_HANDLE;

            vkCmdBeginRenderingKHR(globals->cmdBuffer, &renderingInfo);

            vkCmdBindPipeline(globals->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, globals->compositionPipeline);
            vkCmdBindDescriptorSets(globals->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, globals->compositionPipelineLayout, 0, 5, (VkDescriptorSet[]){globals->PVmatrixdescriptorSet, globals->gbufferDescriptorSet, globals->ssaoAttachmentDescriptorSet, globals->lightDescriptorSet, globals->shadowmapDescriptorSet}, 0, VK_NULL_HANDLE);

            vkCmdDraw(globals->cmdBuffer, 3, 1, 0, 0);

            vkCmdEndRenderingKHR(globals->cmdBuffer);
        }

        {
            VkImageMemoryBarrier imageBarriers[2] = {};
            if (config.bloomEnable) {
                imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                imageBarriers[0].image = globals->blurredPostProcessAttachment;
                imageBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBarriers[0].subresourceRange.baseArrayLayer = 0;
                imageBarriers[0].subresourceRange.layerCount = 1;
                imageBarriers[0].subresourceRange.baseMipLevel = 0;
                imageBarriers[0].subresourceRange.levelCount = 1;
                imageBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageBarriers[0].srcAccessMask = 0;
                imageBarriers[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                vkCmdPipelineBarrier(globals->cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, imageBarriers);
            }

            imageBarriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarriers[1].image = globals->postProcessAttachment;
            imageBarriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarriers[1].subresourceRange.baseArrayLayer = 0;
            imageBarriers[1].subresourceRange.layerCount = 1;
            imageBarriers[1].subresourceRange.baseMipLevel = 0;
            imageBarriers[1].subresourceRange.levelCount = 1;
            imageBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            imageBarriers[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            imageBarriers[1].oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            imageBarriers[1].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            vkCmdPipelineBarrier(globals->cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, imageBarriers + 1);
        }

        if (config.bloomEnable) {
            VkRenderingAttachmentInfoKHR attachments[1] = {};
            attachments[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            attachments[0].imageView = globals->blurredPostProcessAttachmentView;
            attachments[0].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachments[0].resolveMode = VK_RESOLVE_MODE_NONE_KHR;
            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[0].clearValue = (VkClearValue){{{0.0f, 0.0f, 0.0f, 0.0f}}};

            VkRenderingInfoKHR renderingInfo = {};
            renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
            renderingInfo.renderArea = (VkRect2D){(VkOffset2D){0, 0}, (VkExtent2D){vkglobals.swapchainExtent.width, vkglobals.swapchainExtent.height}};
            renderingInfo.layerCount = 1;
            renderingInfo.colorAttachmentCount = 1;
            renderingInfo.pColorAttachments = attachments;
            renderingInfo.pDepthAttachment = VK_NULL_HANDLE;
            renderingInfo.pStencilAttachment = VK_NULL_HANDLE;

            vkCmdBeginRenderingKHR(globals->cmdBuffer, &renderingInfo);

            vkCmdBindPipeline(globals->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, globals->gaussianBlurPipeline);
            vkCmdBindDescriptorSets(globals->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, globals->sampledImagePipelineLayout, 0, 1, (VkDescriptorSet[]){globals->postProcessAttachmentDescriptorSet}, 0, VK_NULL_HANDLE);

            vkCmdDraw(globals->cmdBuffer, 3, 1, 0, 0);

            vkCmdEndRenderingKHR(globals->cmdBuffer);
        }

        {
            VkImageMemoryBarrier imageBarriers[2] = {};
            imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarriers[0].image = globals->swapchainImages[imageIndex];
            imageBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarriers[0].subresourceRange.baseArrayLayer = 0;
            imageBarriers[0].subresourceRange.layerCount = 1;
            imageBarriers[0].subresourceRange.baseMipLevel = 0;
            imageBarriers[0].subresourceRange.levelCount = 1;
            imageBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[0].srcAccessMask = 0;
            imageBarriers[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            if (config.bloomEnable) {
                imageBarriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                imageBarriers[1].image = globals->blurredPostProcessAttachment;
                imageBarriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBarriers[1].subresourceRange.baseArrayLayer = 0;
                imageBarriers[1].subresourceRange.layerCount = 1;
                imageBarriers[1].subresourceRange.baseMipLevel = 0;
                imageBarriers[1].subresourceRange.levelCount = 1;
                imageBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageBarriers[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                imageBarriers[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                imageBarriers[1].oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                imageBarriers[1].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                vkCmdPipelineBarrier(globals->cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, imageBarriers + 1);
            }

            vkCmdPipelineBarrier(globals->cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, imageBarriers);
        }

        {
            VkRenderingAttachmentInfoKHR attachments[1] = {};
            attachments[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            attachments[0].imageView = globals->swapchainImageViews[imageIndex];
            attachments[0].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachments[0].resolveMode = VK_RESOLVE_MODE_NONE_KHR;
            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[0].clearValue = (VkClearValue){{{0.0f, 0.0f, 0.0f, 0.0f}}};

            VkRenderingInfoKHR renderingInfo = {};
            renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
            renderingInfo.renderArea = (VkRect2D){(VkOffset2D){0, 0}, vkglobals.swapchainExtent};
            renderingInfo.layerCount = 1;
            renderingInfo.colorAttachmentCount = 1;
            renderingInfo.pColorAttachments = attachments;
            renderingInfo.pDepthAttachment = VK_NULL_HANDLE;
            renderingInfo.pStencilAttachment = VK_NULL_HANDLE;

            vkCmdBeginRenderingKHR(globals->cmdBuffer, &renderingInfo);

            vkCmdBindPipeline(globals->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, globals->uberPipeline);
            vkCmdBindDescriptorSets(globals->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, globals->uberPipelineLayout, 0, 3, (VkDescriptorSet[]){globals->postProcessAttachmentDescriptorSet, globals->gbufferDescriptorSet, globals->blurredPostProcessAttachmentDescriptorSet}, 0, VK_NULL_HANDLE);

            f32 pcs[2] = {(f32)vkglobals.timeMS / 1e3f, vkglobals.fps};
            vkCmdPushConstants(globals->cmdBuffer, globals->uberPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(f32) * 2, pcs);

            vkCmdDraw(globals->cmdBuffer, 3, 1, 0, 0);

            vkCmdEndRenderingKHR(globals->cmdBuffer);
        }

        {
            VkImageMemoryBarrier imageBarriers[1] = {};
            imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarriers[0].image = globals->swapchainImages[imageIndex];
            imageBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarriers[0].subresourceRange.baseArrayLayer = 0;
            imageBarriers[0].subresourceRange.layerCount = 1;
            imageBarriers[0].subresourceRange.baseMipLevel = 0;
            imageBarriers[0].subresourceRange.levelCount = 1;
            imageBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            imageBarriers[0].dstAccessMask = 0;
            imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            vkCmdPipelineBarrier(globals->cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, imageBarriers);
        }

        VK_ASSERT(vkEndCommandBuffer(globals->cmdBuffer), "failed to end command buffer\n");
    }

    VkPipelineStageFlags semaphoreSignalStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &globals->swapchainReadySemaphore;
    submitInfo.pWaitDstStageMask = &semaphoreSignalStage;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &globals->renderingDoneSemaphores[imageIndex];
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &globals->cmdBuffer;

    VK_ASSERT(vkQueueSubmit(vkglobals.queue, 1, &submitInfo, globals->frameFence), "failed to submit command buffer\n");

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &vkglobals.swapchain;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &globals->renderingDoneSemaphores[imageIndex];
    presentInfo.pImageIndices = &imageIndex;

    VkResult res = vkQueuePresentKHR(vkglobals.queue, &presentInfo);
    if (res != VK_SUBOPTIMAL_KHR) VK_ASSERT(res, "failed to present swapchain image\n");
}

void modelViewSceneQuit() {
    vkDestroyFence(vkglobals.device, globals->frameFence, VK_NULL_HANDLE);

    for (u32 i = 0; i < globals->swapchainImageCount; i++) vkDestroySemaphore(vkglobals.device, globals->renderingDoneSemaphores[i], VK_NULL_HANDLE);
    free(globals->renderingDoneSemaphores);
    
    vkDestroySemaphore(vkglobals.device, globals->swapchainReadySemaphore, VK_NULL_HANDLE);

    vkDestroyPipelineLayout(vkglobals.device, globals->uberPipelineLayout, VK_NULL_HANDLE);
    vkDestroyPipelineLayout(vkglobals.device, globals->compositionPipelineLayout, VK_NULL_HANDLE);
    vkDestroyPipelineLayout(vkglobals.device, globals->PVgbufferPipelineLayout, VK_NULL_HANDLE);
    vkDestroyPipelineLayout(vkglobals.device, globals->PVPipelineLayout, VK_NULL_HANDLE);
    vkDestroyPipelineLayout(vkglobals.device, globals->modelPipelineLayout, VK_NULL_HANDLE);
    vkDestroyPipelineLayout(vkglobals.device, globals->shadowmapPipelineLayout, VK_NULL_HANDLE);
    vkDestroyPipelineLayout(vkglobals.device, globals->sampledImagePipelineLayout, VK_NULL_HANDLE);

    vkDestroyPipeline(vkglobals.device, globals->uberPipeline, VK_NULL_HANDLE);
    vkDestroyPipeline(vkglobals.device, globals->gaussianBlurPipeline, VK_NULL_HANDLE);
    vkDestroyPipeline(vkglobals.device, globals->compositionPipeline, VK_NULL_HANDLE);
    vkDestroyPipeline(vkglobals.device, globals->ssaoPipeline, VK_NULL_HANDLE);
    vkDestroyPipeline(vkglobals.device, globals->skyboxPipeline, VK_NULL_HANDLE);
    vkDestroyPipeline(vkglobals.device, globals->modelPipeline, VK_NULL_HANDLE);
    vkDestroyPipeline(vkglobals.device, globals->shadowmapPipeline, VK_NULL_HANDLE);

    vkDestroyDescriptorSetLayout(vkglobals.device, globals->modelDescriptorSetLayout, VK_NULL_HANDLE);
    vkDestroyDescriptorSetLayout(vkglobals.device, globals->combinedImageSamplerDescriptorSetLayout, VK_NULL_HANDLE);
    vkDestroyDescriptorSetLayout(vkglobals.device, globals->sampledImageDescriptorSetLayout, VK_NULL_HANDLE);
    vkDestroyDescriptorSetLayout(vkglobals.device, globals->gbufferDescriptorSetLayout, VK_NULL_HANDLE);
    vkDestroyDescriptorSetLayout(vkglobals.device, globals->uniformDescriptorSetLayout, VK_NULL_HANDLE);
    vkDestroyDescriptorSetLayout(vkglobals.device, globals->storageDescriptorSetLayout, VK_NULL_HANDLE);
    vkDestroyDescriptorSetLayout(vkglobals.device, globals->PVmatrixdescriptorSetLayout, VK_NULL_HANDLE);

    vkDestroyDescriptorPool(vkglobals.device, globals->descriptorPool, VK_NULL_HANDLE);

    vkDestroyBuffer(vkglobals.device, globals->lightBuffer, VK_NULL_HANDLE);
    vkDestroyBuffer(vkglobals.device, globals->viewMatrixBuffer, VK_NULL_HANDLE);

    vkDestroyImageView(vkglobals.device, globals->blurredPostProcessAttachmentView, VK_NULL_HANDLE);
    vkDestroyImageView(vkglobals.device, globals->postProcessAttachmentView, VK_NULL_HANDLE);
    vkDestroyImageView(vkglobals.device, globals->ssaoAttachmentView, VK_NULL_HANDLE);
    vkDestroyImageView(vkglobals.device, globals->metallicRoughnessVelocityTextureView, VK_NULL_HANDLE);
    vkDestroyImageView(vkglobals.device, globals->gbufferAlbedoView, VK_NULL_HANDLE);
    vkDestroyImageView(vkglobals.device, globals->gbufferNormalView, VK_NULL_HANDLE);
    vkDestroyImageView(vkglobals.device, globals->shadowmapView, VK_NULL_HANDLE);
    vkDestroyImageView(vkglobals.device, globals->depthTextureView, VK_NULL_HANDLE);
    
    vkDestroyImage(vkglobals.device, globals->blurredPostProcessAttachment, VK_NULL_HANDLE);
    vkDestroyImage(vkglobals.device, globals->postProcessAttachment, VK_NULL_HANDLE);
    vkDestroyImage(vkglobals.device, globals->ssaoAttachment, VK_NULL_HANDLE);
    vkDestroyImage(vkglobals.device, globals->metallicRoughnessVelocityTexture, VK_NULL_HANDLE);
    vkDestroyImage(vkglobals.device, globals->gbuffer, VK_NULL_HANDLE);
    vkDestroyImage(vkglobals.device, globals->shadowmap, VK_NULL_HANDLE);
    vkDestroyImage(vkglobals.device, globals->depthTexture, VK_NULL_HANDLE);

    vkDestroyBuffer(vkglobals.device, globals->skyboxVertexBuffer, VK_NULL_HANDLE);
    vkDestroyBuffer(vkglobals.device, globals->projectionBuffer, VK_NULL_HANDLE);

    vkUnmapMemory(vkglobals.device, globals->hostVisibleStorageMemory);
    vkUnmapMemory(vkglobals.device, globals->hostVisibleUniformMemory);

    vkFreeMemory(vkglobals.device, globals->hostVisibleStorageMemory, VK_NULL_HANDLE);
    vkFreeMemory(vkglobals.device, globals->hostVisibleUniformMemory, VK_NULL_HANDLE);

    vkModelDestroy(&globals->model);

    vkFreeMemory(vkglobals.device, globals->deviceLocalIndirectTransferDstMemory, VK_NULL_HANDLE);
    vkFreeMemory(vkglobals.device, globals->deviceLocalIndexTransferDstMemory, VK_NULL_HANDLE);
    vkFreeMemory(vkglobals.device, globals->deviceLocalVertexTransferDstMemory, VK_NULL_HANDLE);
    vkFreeMemory(vkglobals.device, globals->deviceLocalStorageTransferDstMemory, VK_NULL_HANDLE);
    vkFreeMemory(vkglobals.device, globals->deviceLocalUniformTransferDstMemory, VK_NULL_HANDLE);
    vkFreeMemory(vkglobals.device, globals->deviceLocalSampledTransferDstMemory, VK_NULL_HANDLE);
    vkFreeMemory(vkglobals.device, globals->deviceLocalColorAttachmentSampledMemory, VK_NULL_HANDLE);
    vkFreeMemory(vkglobals.device, globals->deviceLocalDepthStencilAttachmentMemory, VK_NULL_HANDLE);

    vkDestroySampler(vkglobals.device, globals->shadowmapSampler, VK_NULL_HANDLE);
    vkDestroySampler(vkglobals.device, globals->sampler, VK_NULL_HANDLE);

    storePipelineCache(globals->pipelineCache, "pipelinecache.dat");
    vkDestroyPipelineCache(vkglobals.device, globals->pipelineCache, VK_NULL_HANDLE);

    for (u32 i = 0; i < globals->swapchainImageCount; i++) vkDestroyImageView(vkglobals.device, globals->swapchainImageViews[i], VK_NULL_HANDLE);
    free(globals->swapchainImages);
}