#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <cglm/cglm.h>
#include <stb_image.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>
#include <math.h>

#include "numtypes.h"
#include "vkFunctions.h"
#include "vkInit.h"
#include "pipeline.h"
#include "game.h"
#include "util.h"
#include "temp.h"
#include "config.h"
#include "mathext.h"
#include "vkModel.h"
game_globals_t gameglobals = {};

#define TEMP_CMD_BUFFER_NUM 1
#define TEMP_BUFFER_NUM 1
#define TEMP_BUFFER_MEMORY_NUM 1
#define TEMP_FENCE_NUM 1

void gameInit() {
    VkCommandBuffer tempCmdBuffers[TEMP_CMD_BUFFER_NUM];
    VkBuffer tempBuffers[TEMP_BUFFER_NUM];
    VkDeviceMemory tempBuffersMemory[TEMP_BUFFER_MEMORY_NUM];
    VkFence tempFences[TEMP_FENCE_NUM];

    tempResourcesCreate(TEMP_CMD_BUFFER_NUM, tempCmdBuffers, TEMP_FENCE_NUM, tempFences);

    {
        #define DEPTH_FORMAT_COUNT 3
        VkFormat formats[DEPTH_FORMAT_COUNT] = {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT};
        
        gameglobals.depthTextureFormat = VK_FORMAT_UNDEFINED;
        for (u32 i = 0; i < DEPTH_FORMAT_COUNT; i++) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(vkglobals.physicalDevice, formats[i], &props);
            if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                gameglobals.depthTextureFormat = formats[i];
            }
        }
        if (gameglobals.depthTextureFormat == VK_FORMAT_UNDEFINED) {
            printf("failed to find suitable depth texture format\n");
            exit(1);
        }
    }

    {
        struct {
            vec3 pos;
            vec3 normal;
            vec2 uv;
            u32 textureIndex;
        } vertexbuf[] = {
             // left face
            {{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, 0},
            {{-1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, 0},
            {{-1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, 0},
            {{-1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, 0},
            {{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, 0},
            {{-1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, 0},
            // back face
            {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 0},
            {{1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, 0},
            {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, 0},
            {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 0},
            {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, 0},
            {{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, 0},
            // top face
            {{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, 2},
            {{1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, 2},
            {{1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, 2},
            {{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, 2},
            {{-1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, 2},
            {{1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, 2},
            // bottom face
            {{-1.0f, 1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, 1},
            {{1.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}, 1},
            {{-1.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}, 1},
            {{-1.0f, 1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, 1},
            {{1.0f, 1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, 1},
            {{1.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}, 1},
            // right face
            {{1.0f, 1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, 0},
            {{1.0f, -1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, 0},
            {{1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, 0},
            {{1.0f, -1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, 0},
            {{1.0f, 1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, 0},
            {{1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, 0},
            // front face
            {{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}, 0},
            {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}, 0},
            {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, 0},
            {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, 0},
            {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}, 0},
            {{1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, 0},
        };

        int32_t wallpaperX, wallpaperY;
        stbi_uc* wallpaperTexture = stbi_load("assets/textures/wallpaper.png", &wallpaperX, &wallpaperY, NULL, STBI_rgb_alpha);

        int32_t carpetX, carpetY;
        stbi_uc* carpetTexture = stbi_load("assets/textures/carpet.png", &carpetX, &carpetY, NULL, STBI_rgb_alpha);

        int32_t ceilingX, ceilingY;
        stbi_uc* ceilingTexture = stbi_load("assets/textures/ceiling.png", &ceilingX, &ceilingY, NULL, STBI_rgb_alpha);

        if (wallpaperX != carpetX || wallpaperX != ceilingX || wallpaperY != carpetY || wallpaperY != ceilingY) {
            printf("wallpaper, carpet and ceiling textures must have identical dimensions\n");
            exit(1);
        }

        char modelPath[256 + 64] = {};
        strcat(modelPath, config.modelDirectoryPath);
        strcat(modelPath, config.modelFile);

        const struct aiScene* scene = vkModelLoadScene(modelPath);

        u32 imagesSize, imageCount;
        vkModelGetTexturesInfo(scene, &imagesSize, &imageCount, NULL, NULL);
        u32 imageWidths[imageCount], imageHeights[imageCount];
        vkModelGetTexturesInfo(scene, &imagesSize, &imageCount, imageWidths, imageHeights);

        gameglobals.model.textures = (VkImage*)malloc((sizeof(VkImage) + sizeof(VkImageView)) * imageCount);
        gameglobals.model.views = (VkImageView*)(gameglobals.model.textures + sizeof(VkImage) * imageCount);
        gameglobals.model.textureCount = imageCount;

        u32 vertexSize, indexSize, indirectSize, storageMaterialsSize, storageMaterialIndicesSize;
        vkModelGetSizes(scene, &vertexSize, &indexSize, &indirectSize, &storageMaterialsSize, &storageMaterialIndicesSize);

        {
            {
                createImage(&gameglobals.depthTexture, vkglobals.swapchainExtent.width, vkglobals.swapchainExtent.height, gameglobals.depthTextureFormat, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

                VkMemoryRequirements memReqs[1];
                vkGetImageMemoryRequirements(vkglobals.device, gameglobals.depthTexture, &memReqs[0]);

                allocateMemory(&gameglobals.deviceLocalDepthStencilAttachmentMemory, memReqs[0].size, getMemoryTypeIndex(memReqs[0].memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

                VK_ASSERT(vkBindImageMemory(vkglobals.device, gameglobals.depthTexture, gameglobals.deviceLocalDepthStencilAttachmentMemory,  0), "failed to bind image memory\n");
            }

            {
                createImage(&gameglobals.cubeTextures, wallpaperX, wallpaperY, VK_FORMAT_R8G8B8A8_UNORM, 3, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
                createImage(&gameglobals.ssaoNoiseTexture, config.ssaoNoiseDim, config.ssaoNoiseDim, VK_FORMAT_R32G32B32A32_SFLOAT, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

                VkMemoryRequirements memReqs[2 + imageCount];
                vkGetImageMemoryRequirements(vkglobals.device, gameglobals.cubeTextures, &memReqs[0]);
                vkGetImageMemoryRequirements(vkglobals.device, gameglobals.ssaoNoiseTexture, &memReqs[1]);

                VkDeviceSize imageOffsets[imageCount + 1];
                imageOffsets[0] = memReqs[0].size + getAlignCooficient(memReqs[0].size, memReqs[1].alignment);
                u32 filter = memReqs[0].memoryTypeBits & memReqs[1].memoryTypeBits;

                for (u32 i = 0; i < imageCount; i++) {
                    createImage(&gameglobals.model.textures[i], imageWidths[i], imageHeights[i], VK_FORMAT_R8G8B8A8_UNORM, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

                    vkGetImageMemoryRequirements(vkglobals.device, gameglobals.model.textures[i], &memReqs[i+2]);

                    imageOffsets[i+1] = imageOffsets[i] + memReqs[i+1].size + getAlignCooficient(memReqs[i+1].size, memReqs[i+2].alignment);

                    filter &= memReqs[i+2].memoryTypeBits;
                }

                allocateMemory(&gameglobals.deviceLocalSampledTransferDstMemory, imageOffsets[imageCount] + memReqs[imageCount + 1].size, getMemoryTypeIndex(filter, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

                VK_ASSERT(vkBindImageMemory(vkglobals.device, gameglobals.cubeTextures, gameglobals.deviceLocalSampledTransferDstMemory,  0), "failed to bind image memory\n");
                VK_ASSERT(vkBindImageMemory(vkglobals.device, gameglobals.ssaoNoiseTexture, gameglobals.deviceLocalSampledTransferDstMemory,  imageOffsets[0]), "failed to bind image memory\n");
                for (u32 i = 0; i < imageCount; i++) VK_ASSERT(vkBindImageMemory(vkglobals.device, gameglobals.model.textures[i], gameglobals.deviceLocalSampledTransferDstMemory,  imageOffsets[i+1]), "failed to bind image memory\n");
            }

            {
                createBuffer(&gameglobals.projectionMatrixBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, sizeof(mat4));
                createBuffer(&gameglobals.ssaoKernelBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, config.ssaoKernelSize * sizeof(vec4));

                VkMemoryRequirements memReqs[2];
                vkGetBufferMemoryRequirements(vkglobals.device, gameglobals.projectionMatrixBuffer, &memReqs[0]);
                vkGetBufferMemoryRequirements(vkglobals.device, gameglobals.ssaoKernelBuffer, &memReqs[1]);

                VkDeviceSize ssaoKernelBufferOffset = memReqs[0].size + getAlignCooficient(memReqs[0].size, memReqs[1].alignment);

                allocateMemory(&gameglobals.deviceLocalUniformTransferDstMemory, ssaoKernelBufferOffset + memReqs[1].size, getMemoryTypeIndex(memReqs[0].memoryTypeBits & memReqs[1].memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

                VK_ASSERT(vkBindBufferMemory(vkglobals.device, gameglobals.projectionMatrixBuffer, gameglobals.deviceLocalUniformTransferDstMemory, 0), "failed to bind buffer memory\n");
                VK_ASSERT(vkBindBufferMemory(vkglobals.device, gameglobals.ssaoKernelBuffer, gameglobals.deviceLocalUniformTransferDstMemory, ssaoKernelBufferOffset), "failed to bind buffer memory\n");
            }

            {
                gameglobals.model.storageBufferMaterialsSize = storageMaterialsSize;
                gameglobals.model.storageBufferMaterialIndicesOffset = storageMaterialsSize + getAlignCooficient(storageMaterialsSize, vkglobals.deviceProperties.limits.minStorageBufferOffsetAlignment);

                createBuffer(&gameglobals.model.storageBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, gameglobals.model.storageBufferMaterialIndicesOffset + storageMaterialIndicesSize);

                VkMemoryRequirements memReqs[1];
                vkGetBufferMemoryRequirements(vkglobals.device, gameglobals.model.storageBuffer, &memReqs[0]);

                allocateMemory(&gameglobals.deviceLocalStorageTransferDstMemory, memReqs[0].size, getMemoryTypeIndex(memReqs[0].memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

                VK_ASSERT(vkBindBufferMemory(vkglobals.device, gameglobals.model.storageBuffer, gameglobals.deviceLocalStorageTransferDstMemory, 0), "failed to bind buffer memory\n");
            }

            {
                createBuffer(&gameglobals.cubeVertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, sizeof(vertexbuf));
                createBuffer(&gameglobals.model.vertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vertexSize);

                VkMemoryRequirements memReqs[2];
                vkGetBufferMemoryRequirements(vkglobals.device, gameglobals.cubeVertexBuffer, &memReqs[0]);
                vkGetBufferMemoryRequirements(vkglobals.device, gameglobals.model.vertexBuffer, &memReqs[1]);

                VkDeviceSize modelVertexBufferOffset = memReqs[0].size + getAlignCooficient(memReqs[0].size, memReqs[1].alignment);

                allocateMemory(&gameglobals.deviceLocalVertexTransferDstMemory, modelVertexBufferOffset + memReqs[1].size, getMemoryTypeIndex(memReqs[0].memoryTypeBits & memReqs[1].memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

                VK_ASSERT(vkBindBufferMemory(vkglobals.device, gameglobals.cubeVertexBuffer, gameglobals.deviceLocalVertexTransferDstMemory, 0), "failed to bind buffer memory\n");
                VK_ASSERT(vkBindBufferMemory(vkglobals.device, gameglobals.model.vertexBuffer, gameglobals.deviceLocalVertexTransferDstMemory, modelVertexBufferOffset), "failed to bind buffer memory\n");
            }

            {
                createBuffer(&gameglobals.model.indexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, indexSize);

                VkMemoryRequirements memReqs[1];
                vkGetBufferMemoryRequirements(vkglobals.device, gameglobals.model.indexBuffer, &memReqs[0]);

                allocateMemory(&gameglobals.deviceLocalIndexTransferDstMemory, memReqs[0].size, getMemoryTypeIndex(memReqs[0].memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

                VK_ASSERT(vkBindBufferMemory(vkglobals.device, gameglobals.model.indexBuffer, gameglobals.deviceLocalIndexTransferDstMemory, 0), "failed to bind buffer memory\n");
            }

            {
                createBuffer(&gameglobals.model.indirectBuffer, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, indirectSize);

                VkMemoryRequirements memReqs[1];
                vkGetBufferMemoryRequirements(vkglobals.device, gameglobals.model.indirectBuffer, &memReqs[0]);

                allocateMemory(&gameglobals.deviceLocalIndirectTransferDstMemory, memReqs[0].size, getMemoryTypeIndex(memReqs[0].memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

                VK_ASSERT(vkBindBufferMemory(vkglobals.device, gameglobals.model.indirectBuffer, gameglobals.deviceLocalIndirectTransferDstMemory, 0), "failed to bind buffer memory\n");
            }
        }

        {
            createBuffer(&tempBuffers[0], VK_BUFFER_USAGE_TRANSFER_SRC_BIT, sizeof(vertexbuf) + wallpaperX * wallpaperY * 4 + carpetX * carpetY * 4 + ceilingX * ceilingY * 4 +
                config.ssaoNoiseDim * config.ssaoNoiseDim * sizeof(vec4) + config.ssaoKernelSize * sizeof(vec4) + sizeof(mat4) + vertexSize + indexSize + indirectSize + gameglobals.model.storageBufferMaterialsSize + storageMaterialIndicesSize + imagesSize);

            VkMemoryRequirements memReq;
            vkGetBufferMemoryRequirements(vkglobals.device, tempBuffers[0], &memReq);

            allocateMemory(&tempBuffersMemory[0], memReq.size, getMemoryTypeIndex(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));

            VK_ASSERT(vkBindBufferMemory(vkglobals.device, tempBuffers[0], tempBuffersMemory[0], 0), "failed to bind buffer memory\n");

            void* tempBufferMemRaw;
            VK_ASSERT(vkMapMemory(vkglobals.device, tempBuffersMemory[0], 0, VK_WHOLE_SIZE, 0, &tempBufferMemRaw), "failed to map memory\n");

            {
                memcpy(tempBufferMemRaw, vertexbuf, sizeof(vertexbuf));
                memcpy(tempBufferMemRaw + sizeof(vertexbuf), wallpaperTexture, wallpaperX * wallpaperY * 4);
                memcpy(tempBufferMemRaw + sizeof(vertexbuf) + wallpaperX * wallpaperY * 4, carpetTexture, carpetX * carpetY * 4);
                memcpy(tempBufferMemRaw + sizeof(vertexbuf) + wallpaperX * wallpaperY * 4 + carpetX * carpetY * 4, ceilingTexture, ceilingX * ceilingY * 4);

                #define tempBufferMemRawWithSSAOoffset (tempBufferMemRaw + sizeof(vertexbuf) + wallpaperX * wallpaperY * 4 + carpetX * carpetY * 4 + ceilingX * ceilingY * 4)
                for (u32 i = 0; i < config.ssaoNoiseDim * config.ssaoNoiseDim; i++) {
                    glm_vec4_copy((vec4){randFloat() * 2.0f - 1.0f, randFloat() * 2.0f - 1.0f, 0.0f, 0.0f}, tempBufferMemRawWithSSAOoffset + i * sizeof(vec4));
                }

                for (u32 i = 0; i < config.ssaoKernelSize; i++) {
                    vec3 sample;
                    glm_vec3_normalize_to((vec3){randFloat() * 2.0f - 1.0f, randFloat() * 2.0f - 1.0f, randFloat()}, sample);
                    glm_vec3_scale(sample, randFloat(), sample);

                    f32 scale = (f32)i / config.ssaoKernelSize;
                    scale = lerpf(0.1f, 1.0f, scale * scale);
                    glm_vec3_scale(sample, scale, sample);
                    glm_vec4_copy((vec4){sample[0], sample[1], sample[2], 0.0f}, tempBufferMemRawWithSSAOoffset + config.ssaoNoiseDim * config.ssaoNoiseDim * sizeof(vec4) + i * sizeof(vec4));
                }

                // use reverse depth
                glm_perspective(glm_rad(config.fov), (f32)vkglobals.swapchainExtent.width / vkglobals.swapchainExtent.height, config.farPlane, config.nearPlane, tempBufferMemRawWithSSAOoffset + config.ssaoNoiseDim * config.ssaoNoiseDim * sizeof(vec4) + config.ssaoKernelSize * sizeof(vec4));

                vkModelCreate(scene, tempCmdBuffers[0], tempBuffers[0], 
                    sizeof(vertexbuf) + wallpaperX * wallpaperY * 4 + carpetX * carpetY * 4 + ceilingX * ceilingY * 4 + config.ssaoNoiseDim * config.ssaoNoiseDim * sizeof(vec4) + config.ssaoKernelSize * sizeof(vec4) + sizeof(mat4),
                    sizeof(vertexbuf) + wallpaperX * wallpaperY * 4 + carpetX * carpetY * 4 + ceilingX * ceilingY * 4 + config.ssaoNoiseDim * config.ssaoNoiseDim * sizeof(vec4) + config.ssaoKernelSize * sizeof(vec4) + sizeof(mat4) + vertexSize,
                    sizeof(vertexbuf) + wallpaperX * wallpaperY * 4 + carpetX * carpetY * 4 + ceilingX * ceilingY * 4 + config.ssaoNoiseDim * config.ssaoNoiseDim * sizeof(vec4) + config.ssaoKernelSize * sizeof(vec4) + sizeof(mat4) + vertexSize + indexSize,
                    sizeof(vertexbuf) + wallpaperX * wallpaperY * 4 + carpetX * carpetY * 4 + ceilingX * ceilingY * 4 + config.ssaoNoiseDim * config.ssaoNoiseDim * sizeof(vec4) + config.ssaoKernelSize * sizeof(vec4) + sizeof(mat4) + vertexSize + indexSize + indirectSize,
                    sizeof(vertexbuf) + wallpaperX * wallpaperY * 4 + carpetX * carpetY * 4 + ceilingX * ceilingY * 4 + config.ssaoNoiseDim * config.ssaoNoiseDim * sizeof(vec4) + config.ssaoKernelSize * sizeof(vec4) + sizeof(mat4) + vertexSize + indexSize + indirectSize + gameglobals.model.storageBufferMaterialsSize + storageMaterialIndicesSize,
                    gameglobals.model.storageBufferMaterialIndicesOffset,
                    tempBufferMemRaw, &gameglobals.model
                );

                VkMappedMemoryRange memoryRange = {};
                memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
                memoryRange.offset = 0;
                memoryRange.size = VK_WHOLE_SIZE;
                memoryRange.memory = tempBuffersMemory[0];

                VK_ASSERT(vkFlushMappedMemoryRanges(vkglobals.device, 1, &memoryRange), "failed to flush device memory\n");
            }
            
            vkUnmapMemory(vkglobals.device, tempBuffersMemory[0]);

            {
                VkBufferCopy copyInfo[3] = {};
                copyInfo[0].srcOffset = 0;
                copyInfo[0].dstOffset = 0;
                copyInfo[0].size = sizeof(vertexbuf);

                copyInfo[1].srcOffset = sizeof(vertexbuf) + wallpaperX * wallpaperY * 4 + carpetX * carpetY * 4 + ceilingX * ceilingY * 4 + config.ssaoNoiseDim * config.ssaoNoiseDim * sizeof(vec4);
                copyInfo[1].dstOffset = 0;
                copyInfo[1].size = config.ssaoKernelSize * sizeof(vec4);

                copyInfo[2].srcOffset = sizeof(vertexbuf) + wallpaperX * wallpaperY * 4 + carpetX * carpetY * 4 + ceilingX * ceilingY * 4 + config.ssaoNoiseDim * config.ssaoNoiseDim * sizeof(vec4) + config.ssaoKernelSize * sizeof(vec4);
                copyInfo[2].dstOffset = 0;
                copyInfo[2].size = sizeof(mat4);

                vkCmdCopyBuffer(tempCmdBuffers[0], tempBuffers[0], gameglobals.cubeVertexBuffer, 1, &copyInfo[0]);
                vkCmdCopyBuffer(tempCmdBuffers[0], tempBuffers[0], gameglobals.ssaoKernelBuffer, 1, &copyInfo[1]);
                vkCmdCopyBuffer(tempCmdBuffers[0], tempBuffers[0], gameglobals.projectionMatrixBuffer, 1, &copyInfo[2]);

                copyTempBufferToImage(tempCmdBuffers[0], tempBuffers[0], sizeof(vertexbuf), gameglobals.cubeTextures, wallpaperX, wallpaperY, 3, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                copyTempBufferToImage(tempCmdBuffers[0], tempBuffers[0],
                    sizeof(vertexbuf) + wallpaperX * wallpaperY * 4 + carpetX * carpetY * 4 + ceilingX * ceilingY * 4,
                    gameglobals.ssaoNoiseTexture, config.ssaoNoiseDim, config.ssaoNoiseDim, 1, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }
        }

        vkModelUnloadScene(scene);
        stbi_image_free(ceilingTexture);
        stbi_image_free(carpetTexture);
        stbi_image_free(wallpaperTexture);
    }

    {
        VkImageMemoryBarrier imageBarrier = {};
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier.image = gameglobals.depthTexture;
        imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        imageBarrier.subresourceRange.baseArrayLayer = 0;
        imageBarrier.subresourceRange.layerCount = 1;
        imageBarrier.subresourceRange.baseMipLevel = 0;
        imageBarrier.subresourceRange.levelCount = 1;
        imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.srcAccessMask = 0;
        imageBarrier.dstAccessMask = 0;
        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        vkCmdPipelineBarrier(tempCmdBuffers[0], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &imageBarrier);
    }

    {
        VK_ASSERT(vkEndCommandBuffer(tempCmdBuffers[0]), "failed to end command buffer\n");

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &tempCmdBuffers[0];

        VK_ASSERT(vkQueueSubmit(vkglobals.queue, 1, &submitInfo, tempFences[0]), "failed to submit command buffer\n");
    }

    {
        {
            createImage(&gameglobals.gbufferPosition, vkglobals.swapchainExtent.width, vkglobals.swapchainExtent.height, VK_FORMAT_R16G16B16A16_SFLOAT, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
            createImage(&gameglobals.gbufferNormalAlbedo, vkglobals.swapchainExtent.width, vkglobals.swapchainExtent.height, VK_FORMAT_R8G8B8A8_UNORM, 2, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
            createImage(&gameglobals.ssaoAttachment, vkglobals.swapchainExtent.width / config.ssaoResolutionFactor, vkglobals.swapchainExtent.height / config.ssaoResolutionFactor, VK_FORMAT_R8_UNORM, 2, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
            createImage(&gameglobals.postProcessAttachment, vkglobals.swapchainExtent.width, vkglobals.swapchainExtent.height, VK_FORMAT_R8G8B8A8_UNORM, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

            VkMemoryRequirements memReqs[4];
            vkGetImageMemoryRequirements(vkglobals.device, gameglobals.gbufferPosition, &memReqs[0]);
            vkGetImageMemoryRequirements(vkglobals.device, gameglobals.gbufferNormalAlbedo, &memReqs[1]);
            vkGetImageMemoryRequirements(vkglobals.device, gameglobals.ssaoAttachment, &memReqs[2]);
            vkGetImageMemoryRequirements(vkglobals.device, gameglobals.postProcessAttachment, &memReqs[3]);

            VkDeviceSize gbufferNormalAlbedoOffset = memReqs[0].size + getAlignCooficient(memReqs[0].size, memReqs[1].alignment);
            VkDeviceSize ssaoAttachmentOffset = gbufferNormalAlbedoOffset + memReqs[1].size + getAlignCooficient(gbufferNormalAlbedoOffset + memReqs[1].size, memReqs[2].alignment);
            VkDeviceSize postProcessAttachmentOffset = ssaoAttachmentOffset + memReqs[2].size + getAlignCooficient(ssaoAttachmentOffset + memReqs[2].size, memReqs[3].alignment);

            allocateMemory(&gameglobals.deviceLocalColorAttachmentSampledMemory, postProcessAttachmentOffset + memReqs[3].size, getMemoryTypeIndex(memReqs[0].memoryTypeBits & memReqs[1].memoryTypeBits & memReqs[2].memoryTypeBits & memReqs[3].memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

            VK_ASSERT(vkBindImageMemory(vkglobals.device, gameglobals.gbufferPosition, gameglobals.deviceLocalColorAttachmentSampledMemory,  0), "failed to bind image memory\n");
            VK_ASSERT(vkBindImageMemory(vkglobals.device, gameglobals.gbufferNormalAlbedo, gameglobals.deviceLocalColorAttachmentSampledMemory,  gbufferNormalAlbedoOffset), "failed to bind image memory\n");
            VK_ASSERT(vkBindImageMemory(vkglobals.device, gameglobals.ssaoAttachment, gameglobals.deviceLocalColorAttachmentSampledMemory,  ssaoAttachmentOffset), "failed to bind image memory\n");
            VK_ASSERT(vkBindImageMemory(vkglobals.device, gameglobals.postProcessAttachment, gameglobals.deviceLocalColorAttachmentSampledMemory,  postProcessAttachmentOffset), "failed to bind image memory\n");
        }
    }

    createImageView(&gameglobals.depthTextureView, gameglobals.depthTexture, VK_IMAGE_VIEW_TYPE_2D, gameglobals.depthTextureFormat, 1, 0, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
    createImageView(&gameglobals.cubeTexturesView, gameglobals.cubeTextures, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_FORMAT_R8G8B8A8_UNORM, 3, 0, VK_IMAGE_ASPECT_COLOR_BIT);
    createImageView(&gameglobals.gbufferPositionView, gameglobals.gbufferPosition, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R16G16B16A16_SFLOAT, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT);
    createImageView(&gameglobals.gbufferNormalView, gameglobals.gbufferNormalAlbedo, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT);
    createImageView(&gameglobals.gbufferAlbedoView, gameglobals.gbufferNormalAlbedo, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT);
    createImageView(&gameglobals.ssaoNoiseTextureView, gameglobals.ssaoNoiseTexture, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT);
    createImageView(&gameglobals.ssaoAttachmentView, gameglobals.ssaoAttachment, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8_UNORM, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT);
    createImageView(&gameglobals.ssaoBlurAttachmentView, gameglobals.ssaoAttachment, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8_UNORM, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT);
    createImageView(&gameglobals.postProcessAttachmentView, gameglobals.postProcessAttachment, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT);

    for (u32 i = 0; i < gameglobals.model.textureCount; i++) createImageView(&gameglobals.model.views[i], gameglobals.model.textures[i], VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, 1, 0, VK_IMAGE_ASPECT_COLOR_BIT);
    
    {
        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.25f;
        samplerInfo.minFilter = vkglobals.textureFilter;
        samplerInfo.magFilter = vkglobals.textureFilter;
        if (config.maxAnisotropy) samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = config.maxAnisotropy;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        VK_ASSERT(vkCreateSampler(vkglobals.device, &samplerInfo, VK_NULL_HANDLE, &gameglobals.sampler), "failed to create sampler\n");
    }

    {
        createBuffer(&gameglobals.modelUniformBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(mat4) * 2);

        VkMemoryRequirements memReqs[1];
        vkGetBufferMemoryRequirements(vkglobals.device, gameglobals.modelUniformBuffer, &memReqs[0]);

        allocateMemory(&gameglobals.hostVisibleUniformMemory, memReqs[0].size, getMemoryTypeIndex(memReqs[0].memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));

        VK_ASSERT(vkBindBufferMemory(vkglobals.device, gameglobals.modelUniformBuffer, gameglobals.hostVisibleUniformMemory, 0), "failed to bind buffer memory\n");

        VK_ASSERT(vkMapMemory(vkglobals.device, gameglobals.hostVisibleUniformMemory, 0, VK_WHOLE_SIZE, 0, &gameglobals.hostVisibleUniformMemoryRaw), "failed to map memory\n");
    }

    {
        VkDescriptorPoolSize poolSizes[3] = {};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = 3;

        poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[1].descriptorCount = 2;

        poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[2].descriptorCount = 8 + gameglobals.model.textureCount;

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 3;
        poolInfo.pPoolSizes = poolSizes;
        poolInfo.maxSets = 8;

        VK_ASSERT(vkCreateDescriptorPool(vkglobals.device, &poolInfo, VK_NULL_HANDLE, &gameglobals.descriptorPool), "failed to create descriptor pool\n");
    }

    {
        {
            VkDescriptorSetLayoutBinding bindings[3] = {};
            bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            bindings[0].binding = 0;
            bindings[0].descriptorCount = 1;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

            bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            bindings[1].binding = 1;
            bindings[1].descriptorCount = 1;
            bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            
            bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings[2].binding = 2;
            bindings[2].descriptorCount = gameglobals.model.textureCount;
            bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

            VkDescriptorBindingFlags bindingFlagsValues[3] = {};
            bindingFlagsValues[2] = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

            VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlags = {};
            bindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
            bindingFlags.bindingCount = 3;
            bindingFlags.pBindingFlags = bindingFlagsValues;

            VkDescriptorSetLayoutCreateInfo layoutInfo = {};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.pNext = &bindingFlags;
            layoutInfo.bindingCount = 3;
            layoutInfo.pBindings = bindings;

            VK_ASSERT(vkCreateDescriptorSetLayout(vkglobals.device, &layoutInfo, VK_NULL_HANDLE, &gameglobals.modelDescriptorSetLayout), "failed to create descriptor set layout\n");
        }

        {
            VkDescriptorSetLayoutBinding bindings[2] = {};
            bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            bindings[0].binding = 0;
            bindings[0].descriptorCount = 1;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

            bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings[1].binding = 1;
            bindings[1].descriptorCount = 1;
            bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
            descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutInfo.bindingCount = 2;
            descriptorSetLayoutInfo.pBindings = bindings;

            VK_ASSERT(vkCreateDescriptorSetLayout(vkglobals.device, &descriptorSetLayoutInfo, VK_NULL_HANDLE, &gameglobals.cubeDescriptorSetLayout), "failed to create descriptor set layout\n");
        }

        {
            VkDescriptorSetLayoutBinding bindings[1] = {};
            bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings[0].binding = 0;
            bindings[0].descriptorCount = 3;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
            descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutInfo.bindingCount = 1;
            descriptorSetLayoutInfo.pBindings = bindings;

            VK_ASSERT(vkCreateDescriptorSetLayout(vkglobals.device, &descriptorSetLayoutInfo, VK_NULL_HANDLE, &gameglobals.gbufferDescriptorSetLayout), "failed to create descriptor set layout\n");
        }

        {
            VkDescriptorSetLayoutBinding bindings[2] = {};
            bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings[0].binding = 0;
            bindings[0].descriptorCount = 1;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

            bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings[1].binding = 1;
            bindings[1].descriptorCount = 1;
            bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
            descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutInfo.bindingCount = 2;
            descriptorSetLayoutInfo.pBindings = bindings;

            VK_ASSERT(vkCreateDescriptorSetLayout(vkglobals.device, &descriptorSetLayoutInfo, VK_NULL_HANDLE, &gameglobals.ssaoDataDescriptorSetLayout), "failed to create descriptor set layout\n");
        }

        {
            VkDescriptorSetLayoutBinding bindings[1] = {};
            bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings[0].binding = 0;
            bindings[0].descriptorCount = 1;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
            descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutInfo.bindingCount = 1;
            descriptorSetLayoutInfo.pBindings = bindings;

            VK_ASSERT(vkCreateDescriptorSetLayout(vkglobals.device, &descriptorSetLayoutInfo, VK_NULL_HANDLE, &gameglobals.projectionMatrixdescriptorSetLayout), "failed to create descriptor set layout\n");
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

            VK_ASSERT(vkCreateDescriptorSetLayout(vkglobals.device, &descriptorSetLayoutInfo, VK_NULL_HANDLE, &gameglobals.sampledImageDescriptorSetLayout), "failed to create descriptor set layout\n");
        }
    }

    {
        {
            u32 variableDescriptorCounts[8] = {};
            variableDescriptorCounts[7] = gameglobals.model.textureCount;

            VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variableDescriptorCountAllocateInfo = {};
            variableDescriptorCountAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
            variableDescriptorCountAllocateInfo.descriptorSetCount = 8;
            variableDescriptorCountAllocateInfo.pDescriptorCounts = variableDescriptorCounts;

            VkDescriptorSetAllocateInfo descriptorSetsInfo = {};
            descriptorSetsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            descriptorSetsInfo.pNext = &variableDescriptorCountAllocateInfo;
            descriptorSetsInfo.descriptorPool = gameglobals.descriptorPool;
            descriptorSetsInfo.descriptorSetCount = 8;
            descriptorSetsInfo.pSetLayouts = (VkDescriptorSetLayout[]){gameglobals.cubeDescriptorSetLayout, gameglobals.gbufferDescriptorSetLayout, gameglobals.ssaoDataDescriptorSetLayout, gameglobals.projectionMatrixdescriptorSetLayout, gameglobals.sampledImageDescriptorSetLayout, gameglobals.sampledImageDescriptorSetLayout, gameglobals.sampledImageDescriptorSetLayout, gameglobals.modelDescriptorSetLayout};

            VkDescriptorSet sets[8];

            VK_ASSERT(vkAllocateDescriptorSets(vkglobals.device, &descriptorSetsInfo, sets), "failed to allocate descriptor sets\n");

            gameglobals.cubeDescriptorSet = sets[0];
            gameglobals.gbufferDescriptorSet = sets[1];
            gameglobals.ssaoDataDescriptorSet = sets[2];
            gameglobals.projectionMatrixdescriptorSet = sets[3];
            gameglobals.ssaoAttachmentDescriptorSet = sets[4];
            gameglobals.ssaoBlurAttachmentDescriptorSet = sets[5];
            gameglobals.postProcessAttachmentDescriptorSet = sets[6];
            gameglobals.model.descriptorSet = sets[7];
        }

        u32 modelDescriptorBufferCount, modelDescriptorImageCount, modelDescriptorWriteCount;
        vkModelGetDescriptorWrites(&gameglobals.model, &modelDescriptorBufferCount, NULL, &modelDescriptorImageCount, NULL, &modelDescriptorWriteCount, NULL);

        VkDescriptorBufferInfo descriptorBufferInfos[3 + modelDescriptorBufferCount];
        descriptorBufferInfos[0].buffer = gameglobals.modelUniformBuffer;
        descriptorBufferInfos[0].offset = 0;
        descriptorBufferInfos[0].range = VK_WHOLE_SIZE;

        descriptorBufferInfos[1].buffer = gameglobals.ssaoKernelBuffer;
        descriptorBufferInfos[1].offset = 0;
        descriptorBufferInfos[1].range = VK_WHOLE_SIZE;

        descriptorBufferInfos[2].buffer = gameglobals.projectionMatrixBuffer;
        descriptorBufferInfos[2].offset = 0;
        descriptorBufferInfos[2].range = VK_WHOLE_SIZE;

        VkDescriptorImageInfo descriptorImageInfos[8 + modelDescriptorImageCount];
        descriptorImageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfos[0].imageView = gameglobals.cubeTexturesView;
        descriptorImageInfos[0].sampler = gameglobals.sampler;

        descriptorImageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfos[1].imageView = gameglobals.gbufferPositionView;
        descriptorImageInfos[1].sampler = gameglobals.sampler;

        descriptorImageInfos[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfos[2].imageView = gameglobals.gbufferNormalView;
        descriptorImageInfos[2].sampler = gameglobals.sampler;

        descriptorImageInfos[3].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfos[3].imageView = gameglobals.gbufferAlbedoView;
        descriptorImageInfos[3].sampler = gameglobals.sampler;

        descriptorImageInfos[4].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfos[4].imageView = gameglobals.ssaoNoiseTextureView;
        descriptorImageInfos[4].sampler = gameglobals.sampler;

        descriptorImageInfos[5].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfos[5].imageView = gameglobals.ssaoAttachmentView;
        descriptorImageInfos[5].sampler = gameglobals.sampler;

        descriptorImageInfos[6].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfos[6].imageView = gameglobals.ssaoBlurAttachmentView;
        descriptorImageInfos[6].sampler = gameglobals.sampler;

        descriptorImageInfos[7].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfos[7].imageView = gameglobals.postProcessAttachmentView;
        descriptorImageInfos[7].sampler = gameglobals.sampler;

        VkWriteDescriptorSet descriptorWrites[11 + modelDescriptorWriteCount];
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].pNext = VK_NULL_HANDLE;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].dstSet = gameglobals.cubeDescriptorSet;
        descriptorWrites[0].pBufferInfo = &descriptorBufferInfos[0];

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].pNext = VK_NULL_HANDLE;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].dstSet = gameglobals.cubeDescriptorSet;
        descriptorWrites[1].pImageInfo = &descriptorImageInfos[0];

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].pNext = VK_NULL_HANDLE;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[2].dstBinding = 0;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].dstSet = gameglobals.gbufferDescriptorSet;
        descriptorWrites[2].pImageInfo = &descriptorImageInfos[1];

        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].pNext = VK_NULL_HANDLE;
        descriptorWrites[3].descriptorCount = 1;
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[3].dstBinding = 0;
        descriptorWrites[3].dstArrayElement = 1;
        descriptorWrites[3].dstSet = gameglobals.gbufferDescriptorSet;
        descriptorWrites[3].pImageInfo = &descriptorImageInfos[2];

        descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[4].pNext = VK_NULL_HANDLE;
        descriptorWrites[4].descriptorCount = 1;
        descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[4].dstBinding = 0;
        descriptorWrites[4].dstArrayElement = 2;
        descriptorWrites[4].dstSet = gameglobals.gbufferDescriptorSet;
        descriptorWrites[4].pImageInfo = &descriptorImageInfos[3];

        descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[5].pNext = VK_NULL_HANDLE;
        descriptorWrites[5].descriptorCount = 1;
        descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[5].dstBinding = 0;
        descriptorWrites[5].dstArrayElement = 0;
        descriptorWrites[5].dstSet = gameglobals.ssaoDataDescriptorSet;
        descriptorWrites[5].pBufferInfo = &descriptorBufferInfos[1];

        descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[6].pNext = VK_NULL_HANDLE;
        descriptorWrites[6].descriptorCount = 1;
        descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[6].dstBinding = 1;
        descriptorWrites[6].dstArrayElement = 0;
        descriptorWrites[6].dstSet = gameglobals.ssaoDataDescriptorSet;
        descriptorWrites[6].pImageInfo = &descriptorImageInfos[4];

        descriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[7].pNext = VK_NULL_HANDLE;
        descriptorWrites[7].descriptorCount = 1;
        descriptorWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[7].dstBinding = 0;
        descriptorWrites[7].dstArrayElement = 0;
        descriptorWrites[7].dstSet = gameglobals.projectionMatrixdescriptorSet;
        descriptorWrites[7].pBufferInfo = &descriptorBufferInfos[2];

        descriptorWrites[8].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[8].pNext = VK_NULL_HANDLE;
        descriptorWrites[8].descriptorCount = 1;
        descriptorWrites[8].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[8].dstBinding = 0;
        descriptorWrites[8].dstArrayElement = 0;
        descriptorWrites[8].dstSet = gameglobals.ssaoAttachmentDescriptorSet;
        descriptorWrites[8].pImageInfo = &descriptorImageInfos[5];

        descriptorWrites[9].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[9].pNext = VK_NULL_HANDLE;
        descriptorWrites[9].descriptorCount = 1;
        descriptorWrites[9].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[9].dstBinding = 0;
        descriptorWrites[9].dstArrayElement = 0;
        descriptorWrites[9].dstSet = gameglobals.ssaoBlurAttachmentDescriptorSet;
        descriptorWrites[9].pImageInfo = &descriptorImageInfos[6];

        descriptorWrites[10].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[10].pNext = VK_NULL_HANDLE;
        descriptorWrites[10].descriptorCount = 1;
        descriptorWrites[10].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[10].dstBinding = 0;
        descriptorWrites[10].dstArrayElement = 0;
        descriptorWrites[10].dstSet = gameglobals.postProcessAttachmentDescriptorSet;
        descriptorWrites[10].pImageInfo = &descriptorImageInfos[7];

        vkModelGetDescriptorWrites(&gameglobals.model, &modelDescriptorBufferCount, descriptorBufferInfos + 3, &modelDescriptorImageCount, descriptorImageInfos + 8, &modelDescriptorWriteCount, descriptorWrites + 11);

        vkUpdateDescriptorSets(vkglobals.device, 11 + modelDescriptorWriteCount, descriptorWrites, 0, VK_NULL_HANDLE);
    }

    {
        {
            VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 3;
            pipelineLayoutInfo.pSetLayouts = (VkDescriptorSetLayout[]){gameglobals.projectionMatrixdescriptorSetLayout, gameglobals.cubeDescriptorSetLayout, gameglobals.modelDescriptorSetLayout};

            VK_ASSERT(vkCreatePipelineLayout(vkglobals.device, &pipelineLayoutInfo, VK_NULL_HANDLE, &gameglobals.modelPipelineLayout), "failed to create pipeline layout\n");
        }

        {
            VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 3;
            pipelineLayoutInfo.pSetLayouts = (VkDescriptorSetLayout[]){gameglobals.projectionMatrixdescriptorSetLayout, gameglobals.ssaoDataDescriptorSetLayout, gameglobals.gbufferDescriptorSetLayout};

            VK_ASSERT(vkCreatePipelineLayout(vkglobals.device, &pipelineLayoutInfo, VK_NULL_HANDLE, &gameglobals.ssaoPipelineLayout), "failed to create pipeline layout\n");
        }

        {
            VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 1;
            pipelineLayoutInfo.pSetLayouts = &gameglobals.sampledImageDescriptorSetLayout;

            VK_ASSERT(vkCreatePipelineLayout(vkglobals.device, &pipelineLayoutInfo, VK_NULL_HANDLE, &gameglobals.sampledImagePipelineLayout), "failed to create pipeline layout\n");
        }

        {
            VkPushConstantRange pcRange = {};
            pcRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            pcRange.size = sizeof(vec3);
            pcRange.offset = 0;

            VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 2;
            pipelineLayoutInfo.pSetLayouts = (VkDescriptorSetLayout[]){gameglobals.gbufferDescriptorSetLayout, gameglobals.sampledImageDescriptorSetLayout};
            pipelineLayoutInfo.pushConstantRangeCount = 1;
            pipelineLayoutInfo.pPushConstantRanges = &pcRange;

            VK_ASSERT(vkCreatePipelineLayout(vkglobals.device, &pipelineLayoutInfo, VK_NULL_HANDLE, &gameglobals.compositionPipelineLayout), "failed to create pipeline layout\n");
        }

        {
            VkPushConstantRange pcRange = {};
            pcRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            pcRange.size = sizeof(f32);
            pcRange.offset = 0;

            VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 1;
            pipelineLayoutInfo.pSetLayouts = &gameglobals.sampledImageDescriptorSetLayout;
            pipelineLayoutInfo.pushConstantRangeCount = 1;
            pipelineLayoutInfo.pPushConstantRanges = &pcRange;

            VK_ASSERT(vkCreatePipelineLayout(vkglobals.device, &pipelineLayoutInfo, VK_NULL_HANDLE, &gameglobals.uberPipelineLayout), "failed to create pipeline layout\n");
        }

        VkSpecializationMapEntry specializationMapEntrys[10] = {};
        specializationMapEntrys[0].constantID = 0;
        specializationMapEntrys[0].offset = 0;
        specializationMapEntrys[0].size = sizeof(u32);
        specializationMapEntrys[1].constantID = 1;
        specializationMapEntrys[1].offset = sizeof(u32);
        specializationMapEntrys[1].size = sizeof(f32);
        specializationMapEntrys[2].constantID = 2;
        specializationMapEntrys[2].offset = sizeof(u32) + sizeof(f32);
        specializationMapEntrys[2].size = sizeof(f32);

        specializationMapEntrys[3].constantID = 2;
        specializationMapEntrys[3].offset = 0;
        specializationMapEntrys[3].size = sizeof(i32);

        specializationMapEntrys[4].constantID = 0;
        specializationMapEntrys[4].offset = 0;
        specializationMapEntrys[4].size = sizeof(u32);
        specializationMapEntrys[5].constantID = 1;
        specializationMapEntrys[5].offset = sizeof(u32);
        specializationMapEntrys[5].size = sizeof(f32);
        specializationMapEntrys[6].constantID = 2;
        specializationMapEntrys[6].offset = sizeof(u32) + sizeof(f32);
        specializationMapEntrys[6].size = sizeof(f32);
        specializationMapEntrys[7].constantID = 3;
        specializationMapEntrys[7].offset = sizeof(u32) + sizeof(f32) * 2;
        specializationMapEntrys[7].size = sizeof(f32);
        specializationMapEntrys[8].constantID = 4;
        specializationMapEntrys[8].offset = sizeof(u32) + sizeof(f32) * 3;
        specializationMapEntrys[8].size = sizeof(u32);
        specializationMapEntrys[9].constantID = 5;
        specializationMapEntrys[9].offset = sizeof(u32) * 2 + sizeof(f32) * 3;
        specializationMapEntrys[9].size = sizeof(f32);

        struct {
            u32 kernelSize;
            f32 radius;
            f32 power;
        } ssaoSpecializationData = {config.ssaoKernelSize, config.ssaoRadius, config.ssaoPower};

        struct {
            i32 blurSize;
        } ssaoBlurSpecializationData = {config.ssaoBlurSize};

        struct {
            u32 grainEnable;
            f32 grainIntensity;
            f32 grainSignalToNoiseRatio;
            f32 grainNoiseShift;

            u32 ditheringEnable;
            f32 ditheringToneCount;
        } uberSpecializationData = {
            config.grainEnable, config.grainIntensity, config.grainSignalToNoise, config.grainNoiseShift,
            config.ditheringEnable, config.ditheringToneCount
        };

        VkSpecializationInfo specializationInfos[3] = {};
        specializationInfos[0].mapEntryCount = 3;
        specializationInfos[0].pMapEntries = specializationMapEntrys;
        specializationInfos[0].dataSize = sizeof(u32) + sizeof(f32) * 2;
        specializationInfos[0].pData = &ssaoSpecializationData;

        specializationInfos[1].mapEntryCount = 1;
        specializationInfos[1].pMapEntries = specializationMapEntrys + 3;
        specializationInfos[1].dataSize = sizeof(i32);
        specializationInfos[1].pData = &ssaoBlurSpecializationData;

        specializationInfos[2].mapEntryCount = 6;
        specializationInfos[2].pMapEntries = specializationMapEntrys + 4;
        specializationInfos[2].dataSize = sizeof(u32) * 2 + sizeof(f32) * 4;
        specializationInfos[2].pData = &uberSpecializationData;

        VkVertexInputBindingDescription bindingDescs[1] = {};
        bindingDescs[0].binding = 0;
        bindingDescs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDescs[0].stride = sizeof(vec3) * 3 + sizeof(vec2);

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

        graphics_pipeline_info_t pipelineInfos[5] = {};
        pipelineFillDefaultGraphicsPipeline(&pipelineInfos[0]);
        pipelineFillDefaultGraphicsPipeline(&pipelineInfos[1]);
        pipelineFillDefaultGraphicsPipeline(&pipelineInfos[2]);
        pipelineFillDefaultGraphicsPipeline(&pipelineInfos[3]);
        pipelineFillDefaultGraphicsPipeline(&pipelineInfos[4]);

        pipelineInfos[0].stageCount = 2;
        pipelineInfos[0].stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        pipelineInfos[0].stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineInfos[0].stages[0].module = createShaderModuleFromAsset("assets/shaders/model.vert.spv");
        pipelineInfos[0].stages[1].module = createShaderModuleFromAsset("assets/shaders/model.frag.spv");

        pipelineInfos[0].vertexInputState.vertexAttributeDescriptionCount = 4;
        pipelineInfos[0].vertexInputState.pVertexAttributeDescriptions = attributeDescs;
        pipelineInfos[0].vertexInputState.vertexBindingDescriptionCount = 1;
        pipelineInfos[0].vertexInputState.pVertexBindingDescriptions = bindingDescs;
        pipelineInfos[0].depthStencilState.depthTestEnable = VK_TRUE;
        pipelineInfos[0].depthStencilState.depthWriteEnable = VK_TRUE;
        pipelineInfos[0].rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        pipelineInfos[0].colorBlendState.attachmentCount = 3;
        pipelineInfos[0].colorBlendState.pAttachments = (VkPipelineColorBlendAttachmentState[]){pipelineInfos[0].colorBlendAttachment, pipelineInfos[0].colorBlendAttachment, pipelineInfos[0].colorBlendAttachment};

        if (config.wireframe) {
            pipelineInfos[0].rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
            // since triangles are transparent in wireframe mode, we disable culling
            pipelineInfos[0].rasterizationState.cullMode = VK_CULL_MODE_NONE;
        }

        pipelineInfos[0].renderingInfo.colorAttachmentCount = 3;
        pipelineInfos[0].renderingInfo.pColorAttachmentFormats = (VkFormat[]){VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM};
        pipelineInfos[0].renderingInfo.depthAttachmentFormat = gameglobals.depthTextureFormat;
        pipelineInfos[0].layout = gameglobals.modelPipelineLayout;


        pipelineInfos[1].stageCount = 2;
        pipelineInfos[1].stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        pipelineInfos[1].stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineInfos[1].stages[0].module = fullscreenVertexModule;
        pipelineInfos[1].stages[1].module = createShaderModuleFromAsset("assets/shaders/ssao.frag.spv");
        pipelineInfos[1].stages[1].pSpecializationInfo = &specializationInfos[0];

        pipelineInfos[1].viewport.width = vkglobals.swapchainExtent.width / config.ssaoResolutionFactor;
        pipelineInfos[1].viewport.height = vkglobals.swapchainExtent.height / config.ssaoResolutionFactor;
        pipelineInfos[1].scissor.extent = (VkExtent2D){vkglobals.swapchainExtent.width / config.ssaoResolutionFactor, vkglobals.swapchainExtent.height / config.ssaoResolutionFactor};

        pipelineInfos[1].renderingInfo.colorAttachmentCount = 1;
        pipelineInfos[1].renderingInfo.pColorAttachmentFormats = (VkFormat[]){VK_FORMAT_R8_UNORM};
        pipelineInfos[1].layout = gameglobals.ssaoPipelineLayout;

        pipelineInfos[2].stageCount = 2;
        pipelineInfos[2].stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        pipelineInfos[2].stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineInfos[2].stages[0].module = fullscreenNoUVVertexModule;
        pipelineInfos[2].stages[1].module = createShaderModuleFromAsset("assets/shaders/ssaoblur.frag.spv");
        pipelineInfos[2].stages[1].pSpecializationInfo = &specializationInfos[1];

        pipelineInfos[2].viewport.width = vkglobals.swapchainExtent.width / config.ssaoResolutionFactor;
        pipelineInfos[2].viewport.height = vkglobals.swapchainExtent.height / config.ssaoResolutionFactor;
        pipelineInfos[2].scissor.extent = (VkExtent2D){vkglobals.swapchainExtent.width / config.ssaoResolutionFactor, vkglobals.swapchainExtent.height / config.ssaoResolutionFactor};

        pipelineInfos[2].renderingInfo.colorAttachmentCount = 1;
        pipelineInfos[2].renderingInfo.pColorAttachmentFormats = (VkFormat[]){VK_FORMAT_R8_UNORM};
        pipelineInfos[2].layout = gameglobals.sampledImagePipelineLayout;

        pipelineInfos[3].stageCount = 2;
        pipelineInfos[3].stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        pipelineInfos[3].stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineInfos[3].stages[0].module = fullscreenVertexModule;
        pipelineInfos[3].stages[1].module = createShaderModuleFromAsset("assets/shaders/composition.frag.spv");

        pipelineInfos[3].renderingInfo.colorAttachmentCount = 1;
        pipelineInfos[3].renderingInfo.pColorAttachmentFormats = (VkFormat[]){VK_FORMAT_R8G8B8A8_UNORM};
        pipelineInfos[3].layout = gameglobals.compositionPipelineLayout;

        pipelineInfos[4].stageCount = 2;
        pipelineInfos[4].stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        pipelineInfos[4].stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineInfos[4].stages[0].module = fullscreenVertexModule;
        pipelineInfos[4].stages[1].module = createShaderModuleFromAsset("assets/shaders/postprocess/uber.frag.spv");
        pipelineInfos[4].stages[1].pSpecializationInfo = &specializationInfos[2];

        pipelineInfos[4].renderingInfo.colorAttachmentCount = 1;
        pipelineInfos[4].renderingInfo.pColorAttachmentFormats = &vkglobals.surfaceFormat.format;
        pipelineInfos[4].layout = gameglobals.uberPipelineLayout;

        VkPipelineCache pipelineCache = loadPipelineCache("pipelinecache.dat");

        VkPipeline pipelines[5];

        pipelineCreateGraphicsPipelines(pipelineCache, 5, pipelineInfos, pipelines);

        gameglobals.modelPipeline = pipelines[0];
        gameglobals.ssaoPipeline = pipelines[1];
        gameglobals.ssaoBlurPipeline = pipelines[2];
        gameglobals.compositionPipeline = pipelines[3];
        gameglobals.uberPipeline = pipelines[4];

        storePipelineCache(pipelineCache, "pipelinecache.dat");
        vkDestroyPipelineCache(vkglobals.device, pipelineCache, VK_NULL_HANDLE);

        vkDestroyShaderModule(vkglobals.device, pipelineInfos[4].stages[1].module, VK_NULL_HANDLE);
        vkDestroyShaderModule(vkglobals.device, pipelineInfos[3].stages[1].module, VK_NULL_HANDLE);
        vkDestroyShaderModule(vkglobals.device, pipelineInfos[2].stages[1].module, VK_NULL_HANDLE);
        vkDestroyShaderModule(vkglobals.device, pipelineInfos[1].stages[1].module, VK_NULL_HANDLE);
        vkDestroyShaderModule(vkglobals.device, pipelineInfos[0].stages[0].module, VK_NULL_HANDLE);
        vkDestroyShaderModule(vkglobals.device, pipelineInfos[0].stages[1].module, VK_NULL_HANDLE);
        vkDestroyShaderModule(vkglobals.device, fullscreenNoUVVertexModule, VK_NULL_HANDLE);
        vkDestroyShaderModule(vkglobals.device, fullscreenVertexModule, VK_NULL_HANDLE);
    }

    {
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        for (u32 i = 0; i < vkglobals.swapchainImageCount; i++) VK_ASSERT(vkCreateSemaphore(vkglobals.device, &semaphoreInfo, VK_NULL_HANDLE, &gameglobals.renderingDoneSemaphores[i]), "failed to create semaphore\n");
        VK_ASSERT(vkCreateSemaphore(vkglobals.device, &semaphoreInfo, VK_NULL_HANDLE, &gameglobals.swapchainReadySemaphore), "failed to create semaphore\n");

        VkFenceCreateInfo fenceInfos[1] = {};
        fenceInfos[0].sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfos[0].flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VK_ASSERT(vkCreateFence(vkglobals.device, &fenceInfos[0], VK_NULL_HANDLE, &gameglobals.frameFence), "failed to create fence\n");
    }

    gameglobals.loopActive = 1;
    gameglobals.deltaTime = 0;
    gameglobals.time = 0;
    gameglobals.shift = 1;

    tempResourcesWaitAndDestroy(TEMP_CMD_BUFFER_NUM, tempCmdBuffers, TEMP_BUFFER_NUM, tempBuffers, TEMP_BUFFER_MEMORY_NUM, tempBuffersMemory, TEMP_FENCE_NUM, tempFences);
}

void gameEvent(SDL_Event* e) {
    if (e->type == SDL_EVENT_KEY_DOWN) {
        if (e->key.key == SDLK_W) gameglobals.inputZ[0] = config.playerSpeed / 2000.0f;
        else if (e->key.key == SDLK_S) gameglobals.inputZ[1] = -config.playerSpeed / 2000.0f;
        else if (e->key.key == SDLK_A) gameglobals.inputX[0] = -config.playerSpeed / 2000.0f;
        else if (e->key.key == SDLK_D) gameglobals.inputX[1] = config.playerSpeed / 2000.0f;
        else if (e->key.key == SDLK_SPACE) gameglobals.inputY[0] = -config.playerSpeed / 2000.0f;
        else if (e->key.key == SDLK_LCTRL) gameglobals.inputY[1] = config.playerSpeed / 2000.0f;
        else if (e->key.key == SDLK_LSHIFT) gameglobals.shift = config.shiftMultiplier;
        else if (e->key.key == SDLK_ESCAPE) gameglobals.loopActive = 0;
    } else if (e->type == SDL_EVENT_KEY_UP) {
        if (e->key.key == SDLK_W) gameglobals.inputZ[0] = 0;
        else if (e->key.key == SDLK_S) gameglobals.inputZ[1] = 0;
        else if (e->key.key == SDLK_A) gameglobals.inputX[0] = 0;
        else if (e->key.key == SDLK_D) gameglobals.inputX[1] = 0;
        else if (e->key.key == SDLK_SPACE) gameglobals.inputY[0] = 0;
        else if (e->key.key == SDLK_LCTRL) gameglobals.inputY[1] = 0;
        else if (e->key.key == SDLK_LSHIFT) gameglobals.shift = 1;
    } else if (e->type == SDL_EVENT_MOUSE_MOTION) {
        gameglobals.cam.yaw += (f32)e->motion.xrel / 400.0f;
        gameglobals.cam.pitch -= (f32)e->motion.yrel / 400.0f;
        clampf(&gameglobals.cam.pitch, -M_PI / 2.0f, M_PI / 2.0f);
    }
}

void updateCubeUbo() {
    #define modelMatrix gameglobals.hostVisibleUniformMemoryRaw
    #define viewMatrix (gameglobals.hostVisibleUniformMemoryRaw + sizeof(mat4))

    versor y, p;
    glm_quatv(y, gameglobals.cam.yaw, (vec3){0.0f, 1.0f, 0.0f});
    glm_quatv(p, gameglobals.cam.pitch, (vec3){1.0f, 0.0f, 0.0f});
    
    {
        mat4 rot;
        glm_quat_mat4(y, rot);
        vec3 vel;
        glm_mat4_mulv3(rot, (vec3){(gameglobals.inputX[0] + gameglobals.inputX[1]) * gameglobals.shift * gameglobals.deltaTime, 0.0f, (gameglobals.inputZ[0] + gameglobals.inputZ[1]) * gameglobals.shift * gameglobals.deltaTime}, 0.0f, vel);
        vel[1] += (gameglobals.inputY[0] + gameglobals.inputY[1]) * gameglobals.shift * gameglobals.deltaTime;
        glm_vec3_add(gameglobals.cam.position, vel, gameglobals.cam.position);
    }

    {
        glm_quat_mul(y, p, y);
        glm_quat_look(gameglobals.cam.position, y, viewMatrix);
    }

    glm_mat4_identity(modelMatrix);
    glm_scale_uni(modelMatrix, config.modelScale);

    VkMappedMemoryRange memoryRange = {};
    memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memoryRange.offset = 0;
    memoryRange.size = VK_WHOLE_SIZE;
    memoryRange.memory = gameglobals.hostVisibleUniformMemory;

    VK_ASSERT(vkFlushMappedMemoryRanges(vkglobals.device, 1, &memoryRange), "failed to flush device memory\n");
}

void gameRender() {
    VK_ASSERT(vkWaitForFences(vkglobals.device, 1, &gameglobals.frameFence, VK_TRUE, 0xFFFFFFFFFFFFFFFF), "failed to wait for fences\n");
    VK_ASSERT(vkResetFences(vkglobals.device, 1, &gameglobals.frameFence), "failed to reset fences\n");

    u32 imageIndex;
    VK_ASSERT(vkAcquireNextImageKHR(vkglobals.device, vkglobals.swapchain, 0xFFFFFFFFFFFFFFFF, gameglobals.swapchainReadySemaphore, VK_NULL_HANDLE, &imageIndex), "failed to acquire swapchain image\n");

    updateCubeUbo();

    {
        VkCommandBufferBeginInfo cmdBeginInfo = {};
        cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_ASSERT(vkBeginCommandBuffer(vkglobals.cmdBuffer, &cmdBeginInfo), "failed to begin command buffer\n");

        {
            VkImageMemoryBarrier imageBarriers[2] = {};
            imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarriers[0].image = gameglobals.gbufferPosition;
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
            imageBarriers[1].image = gameglobals.gbufferNormalAlbedo;
            imageBarriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarriers[1].subresourceRange.baseArrayLayer = 0;
            imageBarriers[1].subresourceRange.layerCount = 2;
            imageBarriers[1].subresourceRange.baseMipLevel = 0;
            imageBarriers[1].subresourceRange.levelCount = 1;
            imageBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[1].srcAccessMask = 0;
            imageBarriers[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            imageBarriers[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageBarriers[1].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            vkCmdPipelineBarrier(vkglobals.cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 2, imageBarriers);
        }

        {
            VkRenderingAttachmentInfoKHR attachments[4] = {};
            attachments[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            attachments[0].imageView = gameglobals.gbufferPositionView;
            attachments[0].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachments[0].resolveMode = VK_RESOLVE_MODE_NONE_KHR;
            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[0].clearValue = (VkClearValue){{{0.0f, 0.0f, 0.0f, 0.0f}}};

            attachments[1].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            attachments[1].imageView = gameglobals.gbufferNormalView;
            attachments[1].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachments[1].resolveMode = VK_RESOLVE_MODE_NONE_KHR;
            attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[1].clearValue = (VkClearValue){{{0.0f, 0.0f, 0.0f, 0.0f}}};

            attachments[2].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            attachments[2].imageView = gameglobals.gbufferAlbedoView;
            attachments[2].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachments[2].resolveMode = VK_RESOLVE_MODE_NONE_KHR;
            attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[2].clearValue = (VkClearValue){{{0.0f, 0.0f, 0.0f, 0.0f}}};

            attachments[3].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            attachments[3].imageView = gameglobals.depthTextureView;
            attachments[3].imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachments[3].resolveMode = VK_RESOLVE_MODE_NONE_KHR;
            attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[3].clearValue = (VkClearValue){{{0.0f, 0}}};

            VkRenderingInfoKHR renderingInfo = {};
            renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
            renderingInfo.renderArea = (VkRect2D){(VkOffset2D){0, 0}, vkglobals.swapchainExtent};
            renderingInfo.layerCount = 1;
            renderingInfo.colorAttachmentCount = 3;
            renderingInfo.pColorAttachments = attachments;
            renderingInfo.pDepthAttachment = &attachments[3];
            renderingInfo.pStencilAttachment = VK_NULL_HANDLE;

            vkCmdBeginRenderingKHR(vkglobals.cmdBuffer, &renderingInfo);
        }

        VkDeviceSize vertexBufferOffsets[1] = {};
        vkCmdBindPipeline(vkglobals.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gameglobals.modelPipeline);
        vkCmdBindVertexBuffers(vkglobals.cmdBuffer, 0, 1, &gameglobals.model.vertexBuffer, vertexBufferOffsets);
        vkCmdBindIndexBuffer(vkglobals.cmdBuffer, gameglobals.model.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(vkglobals.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gameglobals.modelPipelineLayout, 0, 3, (VkDescriptorSet[]){gameglobals.projectionMatrixdescriptorSet, gameglobals.cubeDescriptorSet, gameglobals.model.descriptorSet}, 0, VK_NULL_HANDLE);

        vkCmdDrawIndexedIndirect(vkglobals.cmdBuffer, gameglobals.model.indirectBuffer, 0, gameglobals.model.drawCount, sizeof(VkDrawIndexedIndirectCommand));

        vkCmdEndRenderingKHR(vkglobals.cmdBuffer);

        {
            VkImageMemoryBarrier imageBarriers[3] = {};
            imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarriers[0].image = gameglobals.ssaoAttachment;
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
            imageBarriers[1].image = gameglobals.gbufferPosition;
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
            imageBarriers[2].image = gameglobals.gbufferNormalAlbedo;
            imageBarriers[2].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarriers[2].subresourceRange.baseArrayLayer = 0;
            imageBarriers[2].subresourceRange.layerCount = 2;
            imageBarriers[2].subresourceRange.baseMipLevel = 0;
            imageBarriers[2].subresourceRange.levelCount = 1;
            imageBarriers[2].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[2].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            imageBarriers[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            imageBarriers[2].oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            imageBarriers[2].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
           
            vkCmdPipelineBarrier(vkglobals.cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &imageBarriers[0]);
            vkCmdPipelineBarrier(vkglobals.cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 2, imageBarriers + 1);
        }

        {
            VkRenderingAttachmentInfoKHR attachments[1] = {};
            attachments[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            attachments[0].imageView = gameglobals.ssaoAttachmentView;
            attachments[0].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachments[0].resolveMode = VK_RESOLVE_MODE_NONE_KHR;
            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[0].clearValue = (VkClearValue){{{0.0f, 0.0f, 0.0f, 0.0f}}};

            VkRenderingInfoKHR renderingInfo = {};
            renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
            renderingInfo.renderArea = (VkRect2D){(VkOffset2D){0, 0}, (VkExtent2D){vkglobals.swapchainExtent.width / config.ssaoResolutionFactor, vkglobals.swapchainExtent.height / config.ssaoResolutionFactor}};
            renderingInfo.layerCount = 1;
            renderingInfo.colorAttachmentCount = 1;
            renderingInfo.pColorAttachments = attachments;
            renderingInfo.pDepthAttachment = VK_NULL_HANDLE;
            renderingInfo.pStencilAttachment = VK_NULL_HANDLE;

            vkCmdBeginRenderingKHR(vkglobals.cmdBuffer, &renderingInfo);
        }

        vkCmdBindPipeline(vkglobals.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gameglobals.ssaoPipeline);
        vkCmdBindDescriptorSets(vkglobals.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gameglobals.ssaoPipelineLayout, 0, 3, (VkDescriptorSet[]){gameglobals.projectionMatrixdescriptorSet, gameglobals.ssaoDataDescriptorSet, gameglobals.gbufferDescriptorSet}, 0, VK_NULL_HANDLE);

        vkCmdDraw(vkglobals.cmdBuffer, 3, 1, 0, 0);

        vkCmdEndRenderingKHR(vkglobals.cmdBuffer);

        {
            VkImageMemoryBarrier imageBarriers[2] = {};
            imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarriers[0].image = gameglobals.ssaoAttachment;
            imageBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarriers[0].subresourceRange.baseArrayLayer = 1;
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
            imageBarriers[1].image = gameglobals.ssaoAttachment;
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

            vkCmdPipelineBarrier(vkglobals.cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &imageBarriers[0]);
            vkCmdPipelineBarrier(vkglobals.cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &imageBarriers[1]);
        }

        {
            VkRenderingAttachmentInfoKHR attachments[1] = {};
            attachments[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            attachments[0].imageView = gameglobals.ssaoBlurAttachmentView;
            attachments[0].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachments[0].resolveMode = VK_RESOLVE_MODE_NONE_KHR;
            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[0].clearValue = (VkClearValue){{{0.0f, 0.0f, 0.0f, 0.0f}}};

            VkRenderingInfoKHR renderingInfo = {};
            renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
            renderingInfo.renderArea = (VkRect2D){(VkOffset2D){0, 0}, (VkExtent2D){vkglobals.swapchainExtent.width / config.ssaoResolutionFactor, vkglobals.swapchainExtent.height / config.ssaoResolutionFactor}};
            renderingInfo.layerCount = 1;
            renderingInfo.colorAttachmentCount = 1;
            renderingInfo.pColorAttachments = attachments;
            renderingInfo.pDepthAttachment = VK_NULL_HANDLE;
            renderingInfo.pStencilAttachment = VK_NULL_HANDLE;

            vkCmdBeginRenderingKHR(vkglobals.cmdBuffer, &renderingInfo);
        }

        vkCmdBindPipeline(vkglobals.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gameglobals.ssaoBlurPipeline);
        vkCmdBindDescriptorSets(vkglobals.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gameglobals.sampledImagePipelineLayout, 0, 1, &gameglobals.ssaoAttachmentDescriptorSet, 0, VK_NULL_HANDLE);

        vkCmdDraw(vkglobals.cmdBuffer, 3, 1, 0, 0);

        vkCmdEndRenderingKHR(vkglobals.cmdBuffer);

        {
            VkImageMemoryBarrier imageBarriers[2] = {};
            imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarriers[0].image = gameglobals.postProcessAttachment;
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
            imageBarriers[1].image = gameglobals.ssaoAttachment;
            imageBarriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarriers[1].subresourceRange.baseArrayLayer = 1;
            imageBarriers[1].subresourceRange.layerCount = 1;
            imageBarriers[1].subresourceRange.baseMipLevel = 0;
            imageBarriers[1].subresourceRange.levelCount = 1;
            imageBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            imageBarriers[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            imageBarriers[1].oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            imageBarriers[1].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            vkCmdPipelineBarrier(vkglobals.cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &imageBarriers[0]);
            vkCmdPipelineBarrier(vkglobals.cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &imageBarriers[1]);
        }

        {
            VkRenderingAttachmentInfoKHR attachments[1] = {};
            attachments[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            attachments[0].imageView = gameglobals.postProcessAttachmentView;
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

            vkCmdBeginRenderingKHR(vkglobals.cmdBuffer, &renderingInfo);
        }

        vkCmdBindPipeline(vkglobals.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gameglobals.compositionPipeline);
        vkCmdBindDescriptorSets(vkglobals.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gameglobals.compositionPipelineLayout, 0, 2, (VkDescriptorSet[]){gameglobals.gbufferDescriptorSet, gameglobals.ssaoBlurAttachmentDescriptorSet}, 0, VK_NULL_HANDLE);

        vec3 viewLightPos;
        glm_mat4_mulv3(gameglobals.hostVisibleUniformMemoryRaw + sizeof(mat4), (vec3){0.0, -0.1, 0.0}, 1.0, viewLightPos);
        vkCmdPushConstants(vkglobals.cmdBuffer, gameglobals.compositionPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(vec3), viewLightPos);

        vkCmdDraw(vkglobals.cmdBuffer, 3, 1, 0, 0);

        vkCmdEndRenderingKHR(vkglobals.cmdBuffer);

        {
            VkImageMemoryBarrier imageBarriers[2] = {};
            imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarriers[0].image = vkglobals.swapchainImages[imageIndex];
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
            imageBarriers[1].image = gameglobals.postProcessAttachment;
            imageBarriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarriers[1].subresourceRange.baseArrayLayer = 0;
            imageBarriers[1].subresourceRange.layerCount = 1;
            imageBarriers[1].subresourceRange.baseMipLevel = 0;
            imageBarriers[1].subresourceRange.levelCount = 1;
            imageBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            imageBarriers[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
            imageBarriers[1].oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            imageBarriers[1].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            vkCmdPipelineBarrier(vkglobals.cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &imageBarriers[0]);
            vkCmdPipelineBarrier(vkglobals.cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &imageBarriers[1]);
        }

        {
            VkRenderingAttachmentInfoKHR attachments[1] = {};
            attachments[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            attachments[0].imageView = vkglobals.swapchainImageViews[imageIndex];
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

            vkCmdBeginRenderingKHR(vkglobals.cmdBuffer, &renderingInfo);
        }

        vkCmdBindPipeline(vkglobals.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gameglobals.uberPipeline);
        vkCmdBindDescriptorSets(vkglobals.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gameglobals.uberPipelineLayout, 0, 1, &gameglobals.postProcessAttachmentDescriptorSet, 0, VK_NULL_HANDLE);

        f32 t = (f32)gameglobals.time / 1000.0f;
        vkCmdPushConstants(vkglobals.cmdBuffer, gameglobals.uberPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(f32), &t);

        vkCmdDraw(vkglobals.cmdBuffer, 3, 1, 0, 0);

        vkCmdEndRenderingKHR(vkglobals.cmdBuffer);

        {
            VkImageMemoryBarrier imageBarriers[1] = {};
            imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarriers[0].image = vkglobals.swapchainImages[imageIndex];
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

            vkCmdPipelineBarrier(vkglobals.cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &imageBarriers[0]);
        }

        VK_ASSERT(vkEndCommandBuffer(vkglobals.cmdBuffer), "failed to end command buffer\n");
    }

    VkPipelineStageFlags semaphoreSignalStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &gameglobals.swapchainReadySemaphore;
    submitInfo.pWaitDstStageMask = &semaphoreSignalStage;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &gameglobals.renderingDoneSemaphores[imageIndex];
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkglobals.cmdBuffer;

    VK_ASSERT(vkQueueSubmit(vkglobals.queue, 1, &submitInfo, gameglobals.frameFence), "failed to submit command buffer\n");

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &vkglobals.swapchain;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &gameglobals.renderingDoneSemaphores[imageIndex];
    presentInfo.pImageIndices = &imageIndex;

    VK_ASSERT(vkQueuePresentKHR(vkglobals.queue, &presentInfo), "failed to present swapchain image\n");
}

void gameQuit() {
    vkDestroyFence(vkglobals.device, gameglobals.frameFence, VK_NULL_HANDLE);

    for (u32 i = 0; i < vkglobals.swapchainImageCount; i++) vkDestroySemaphore(vkglobals.device, gameglobals.renderingDoneSemaphores[i], VK_NULL_HANDLE);
    vkDestroySemaphore(vkglobals.device, gameglobals.swapchainReadySemaphore, VK_NULL_HANDLE);

    vkDestroyPipelineLayout(vkglobals.device, gameglobals.uberPipelineLayout, VK_NULL_HANDLE);
    vkDestroyPipelineLayout(vkglobals.device, gameglobals.compositionPipelineLayout, VK_NULL_HANDLE);
    vkDestroyPipelineLayout(vkglobals.device, gameglobals.sampledImagePipelineLayout, VK_NULL_HANDLE);
    vkDestroyPipelineLayout(vkglobals.device, gameglobals.ssaoPipelineLayout, VK_NULL_HANDLE);
    vkDestroyPipelineLayout(vkglobals.device, gameglobals.modelPipelineLayout, VK_NULL_HANDLE);

    vkDestroyPipeline(vkglobals.device, gameglobals.uberPipeline, VK_NULL_HANDLE);
    vkDestroyPipeline(vkglobals.device, gameglobals.compositionPipeline, VK_NULL_HANDLE);
    vkDestroyPipeline(vkglobals.device, gameglobals.ssaoBlurPipeline, VK_NULL_HANDLE);
    vkDestroyPipeline(vkglobals.device, gameglobals.ssaoPipeline, VK_NULL_HANDLE);
    vkDestroyPipeline(vkglobals.device, gameglobals.modelPipeline, VK_NULL_HANDLE);

    vkDestroyDescriptorSetLayout(vkglobals.device, gameglobals.modelDescriptorSetLayout, VK_NULL_HANDLE);
    vkDestroyDescriptorSetLayout(vkglobals.device, gameglobals.sampledImageDescriptorSetLayout, VK_NULL_HANDLE);
    vkDestroyDescriptorSetLayout(vkglobals.device, gameglobals.gbufferDescriptorSetLayout, VK_NULL_HANDLE);
    vkDestroyDescriptorSetLayout(vkglobals.device, gameglobals.ssaoDataDescriptorSetLayout, VK_NULL_HANDLE);
    vkDestroyDescriptorSetLayout(vkglobals.device, gameglobals.cubeDescriptorSetLayout, VK_NULL_HANDLE);
    vkDestroyDescriptorSetLayout(vkglobals.device, gameglobals.projectionMatrixdescriptorSetLayout, VK_NULL_HANDLE);

    vkDestroyDescriptorPool(vkglobals.device, gameglobals.descriptorPool, VK_NULL_HANDLE);

    vkDestroyBuffer(vkglobals.device, gameglobals.modelUniformBuffer, VK_NULL_HANDLE);

    vkDestroyImageView(vkglobals.device, gameglobals.postProcessAttachmentView, VK_NULL_HANDLE);
    vkDestroyImageView(vkglobals.device, gameglobals.ssaoBlurAttachmentView, VK_NULL_HANDLE);
    vkDestroyImageView(vkglobals.device, gameglobals.ssaoAttachmentView, VK_NULL_HANDLE);
    vkDestroyImageView(vkglobals.device, gameglobals.ssaoNoiseTextureView, VK_NULL_HANDLE);
    vkDestroyImageView(vkglobals.device, gameglobals.gbufferNormalView, VK_NULL_HANDLE);
    vkDestroyImageView(vkglobals.device, gameglobals.gbufferPositionView, VK_NULL_HANDLE);
    vkDestroyImageView(vkglobals.device, gameglobals.gbufferAlbedoView, VK_NULL_HANDLE);
    vkDestroyImageView(vkglobals.device, gameglobals.cubeTexturesView, VK_NULL_HANDLE);
    vkDestroyImageView(vkglobals.device, gameglobals.depthTextureView, VK_NULL_HANDLE);
    
    vkDestroyImage(vkglobals.device, gameglobals.postProcessAttachment, VK_NULL_HANDLE);
    vkDestroyImage(vkglobals.device, gameglobals.ssaoAttachment, VK_NULL_HANDLE);
    vkDestroyImage(vkglobals.device, gameglobals.ssaoNoiseTexture, VK_NULL_HANDLE);
    vkDestroyImage(vkglobals.device, gameglobals.gbufferNormalAlbedo, VK_NULL_HANDLE);
    vkDestroyImage(vkglobals.device, gameglobals.gbufferPosition, VK_NULL_HANDLE);
    vkDestroyImage(vkglobals.device, gameglobals.cubeTextures, VK_NULL_HANDLE);
    vkDestroyImage(vkglobals.device, gameglobals.depthTexture, VK_NULL_HANDLE);

    vkDestroyBuffer(vkglobals.device, gameglobals.cubeVertexBuffer, VK_NULL_HANDLE);
    vkDestroyBuffer(vkglobals.device, gameglobals.ssaoKernelBuffer, VK_NULL_HANDLE);
    vkDestroyBuffer(vkglobals.device, gameglobals.projectionMatrixBuffer, VK_NULL_HANDLE);

    vkUnmapMemory(vkglobals.device, gameglobals.hostVisibleUniformMemory);

    vkFreeMemory(vkglobals.device, gameglobals.hostVisibleUniformMemory, VK_NULL_HANDLE);

    vkModelDestroyModel(&gameglobals.model);

    vkFreeMemory(vkglobals.device, gameglobals.deviceLocalIndirectTransferDstMemory, VK_NULL_HANDLE);
    vkFreeMemory(vkglobals.device, gameglobals.deviceLocalIndexTransferDstMemory, VK_NULL_HANDLE);
    vkFreeMemory(vkglobals.device, gameglobals.deviceLocalVertexTransferDstMemory, VK_NULL_HANDLE);
    vkFreeMemory(vkglobals.device, gameglobals.deviceLocalStorageTransferDstMemory, VK_NULL_HANDLE);
    vkFreeMemory(vkglobals.device, gameglobals.deviceLocalUniformTransferDstMemory, VK_NULL_HANDLE);
    vkFreeMemory(vkglobals.device, gameglobals.deviceLocalSampledTransferDstMemory, VK_NULL_HANDLE);
    vkFreeMemory(vkglobals.device, gameglobals.deviceLocalColorAttachmentSampledMemory, VK_NULL_HANDLE);
    vkFreeMemory(vkglobals.device, gameglobals.deviceLocalDepthStencilAttachmentMemory, VK_NULL_HANDLE);

    vkDestroySampler(vkglobals.device, gameglobals.sampler, VK_NULL_HANDLE);
}