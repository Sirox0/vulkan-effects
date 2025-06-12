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
#include "garbage.h"
#include "config.h"
#include "mathext.h"

game_globals_t gameglobals = {};

#define GARBAGE_CMD_BUFFER_NUM 1
#define GARBAGE_BUFFER_NUM 1
#define GARBAGE_BUFFER_MEMORY_NUM 1
#define GARBAGE_FENCE_NUM 1

#define DEPTH_FORMAT_COUNT 3

void gameInit() {
    VkCommandBuffer garbageCmdBuffers[GARBAGE_CMD_BUFFER_NUM];
    VkBuffer garbageBuffers[GARBAGE_BUFFER_NUM];
    VkDeviceMemory garbageBuffersMem[GARBAGE_BUFFER_MEMORY_NUM];
    VkFence garbageFences[GARBAGE_FENCE_NUM];

    garbageCreate(GARBAGE_CMD_BUFFER_NUM, garbageCmdBuffers, GARBAGE_FENCE_NUM, garbageFences);

    {
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
        vec3 vertexbuf[] = {
            {-2.0f, 2.0f, 2.0f},
            {-2.0f, 2.0f, -2.0f},
            {2.0f, 2.0f, -2.0f},
            {2.0f, 2.0f, 2.0f},
            {-2.0f, -2.0f, 2.0f},
            {-2.0f, -2.0f, -2.0f},
            {2.0f, -2.0f, -2.0f},
            {2.0f, -2.0f, 2.0f},
        };

        u16 indexbuf[] = {
            1, 2, 0,
            2, 3, 0,

            5, 4, 6,
            6, 4, 7,

            4, 5, 0,
            5, 1, 0,

            7, 3, 6,
            6, 3, 2,

            6, 2, 5,
            5, 2, 1,

            4, 3, 7,
            4, 0, 3
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

        createImage(&gameglobals.depthTexture, vkglobals.swapchainExtent.width, vkglobals.swapchainExtent.height, gameglobals.depthTextureFormat, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
        createImage(&gameglobals.gameTextures, wallpaperX, wallpaperY, VK_FORMAT_R8G8B8A8_UNORM, 3, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        createBuffer(&gameglobals.cubeVertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, sizeof(vertexbuf));
        createBuffer(&gameglobals.cubeIndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, sizeof(indexbuf));

        VkMemoryRequirements depthTextureMemReq;
        vkGetImageMemoryRequirements(vkglobals.device, gameglobals.depthTexture, &depthTextureMemReq);
        VkMemoryRequirements gameTexturesMemReq;
        vkGetImageMemoryRequirements(vkglobals.device, gameglobals.gameTextures, &gameTexturesMemReq);
        VkMemoryRequirements vertexBufferMemReq;
        vkGetBufferMemoryRequirements(vkglobals.device, gameglobals.cubeVertexBuffer, &vertexBufferMemReq);
        VkMemoryRequirements indexBufferMemReq;
        vkGetBufferMemoryRequirements(vkglobals.device, gameglobals.cubeIndexBuffer, &indexBufferMemReq);

        gameglobals.gameTexturesOffset = depthTextureMemReq.size + getAlignCooficient(depthTextureMemReq.size, gameTexturesMemReq.alignment);
        gameglobals.cubeVertexBufferOffset = gameglobals.gameTexturesOffset + gameTexturesMemReq.size + getAlignCooficient(gameglobals.gameTexturesOffset + gameTexturesMemReq.size, vertexBufferMemReq.alignment);
        gameglobals.cubeIndexBufferOffset = gameglobals.cubeVertexBufferOffset + vertexBufferMemReq.size + getAlignCooficient(gameglobals.cubeVertexBufferOffset + vertexBufferMemReq.size, indexBufferMemReq.alignment);

        allocateMemory(&gameglobals.deviceLocalMemory, gameglobals.cubeIndexBufferOffset + indexBufferMemReq.size, getMemoryTypeIndex(depthTextureMemReq.memoryTypeBits & gameTexturesMemReq.memoryTypeBits & vertexBufferMemReq.memoryTypeBits & indexBufferMemReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

        VK_ASSERT(vkBindImageMemory(vkglobals.device, gameglobals.depthTexture, gameglobals.deviceLocalMemory,  0), "failed to bind image memory\n");
        VK_ASSERT(vkBindImageMemory(vkglobals.device, gameglobals.gameTextures, gameglobals.deviceLocalMemory,  gameglobals.gameTexturesOffset), "failed to bind image memory\n");
        VK_ASSERT(vkBindBufferMemory(vkglobals.device, gameglobals.cubeVertexBuffer, gameglobals.deviceLocalMemory, gameglobals.cubeVertexBufferOffset), "failed to bind buffer memory\n");
        VK_ASSERT(vkBindBufferMemory(vkglobals.device, gameglobals.cubeIndexBuffer, gameglobals.deviceLocalMemory, gameglobals.cubeIndexBufferOffset), "failed to bind buffer memory\n");

        {
            createBuffer(&garbageBuffers[0], VK_BUFFER_USAGE_TRANSFER_SRC_BIT, sizeof(vertexbuf) + sizeof(indexbuf) + wallpaperX * wallpaperY * 4 + carpetX * carpetY * 4 + ceilingX * ceilingY * 4);

            VkMemoryRequirements garbageBufferMemReq;
            vkGetBufferMemoryRequirements(vkglobals.device, garbageBuffers[0], &garbageBufferMemReq);

            allocateMemory(&garbageBuffersMem[0], garbageBufferMemReq.size, getMemoryTypeIndex(garbageBufferMemReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));

            VK_ASSERT(vkBindBufferMemory(vkglobals.device, garbageBuffers[0], garbageBuffersMem[0], 0), "failed to bind buffer memory\n");

            void* garbageBuffer0MemRaw;
            VK_ASSERT(vkMapMemory(vkglobals.device, garbageBuffersMem[0], 0, VK_WHOLE_SIZE, 0, &garbageBuffer0MemRaw), "failed to map memory\n");

            memcpy(garbageBuffer0MemRaw, vertexbuf, sizeof(vertexbuf));
            memcpy(garbageBuffer0MemRaw + sizeof(vertexbuf), indexbuf, sizeof(indexbuf));
            memcpy(garbageBuffer0MemRaw + sizeof(vertexbuf) + sizeof(indexbuf), wallpaperTexture, wallpaperX * wallpaperY * 4);
            memcpy(garbageBuffer0MemRaw + sizeof(vertexbuf) + sizeof(indexbuf) + wallpaperX * wallpaperY * 4, carpetTexture, carpetX * carpetY * 4);
            memcpy(garbageBuffer0MemRaw + sizeof(vertexbuf) + sizeof(indexbuf) + wallpaperX * wallpaperY * 4 + carpetX * carpetY * 4, ceilingTexture, ceilingX * ceilingY * 4);

            VkMappedMemoryRange memoryRange = {};
            memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            memoryRange.offset = 0;
            memoryRange.size = VK_WHOLE_SIZE;
            memoryRange.memory = garbageBuffersMem[0];

            VK_ASSERT(vkFlushMappedMemoryRanges(vkglobals.device, 1, &memoryRange), "failed to flush device memory\n");
            
            vkUnmapMemory(vkglobals.device, garbageBuffersMem[0]);

            VkBufferCopy copyInfo[2] = {};
            copyInfo[0].srcOffset = 0;
            copyInfo[0].dstOffset = 0;
            copyInfo[0].size = vertexBufferMemReq.size;
            copyInfo[1].srcOffset = sizeof(vertexbuf);
            copyInfo[1].dstOffset = 0;
            copyInfo[1].size = indexBufferMemReq.size;

            vkCmdCopyBuffer(garbageCmdBuffers[0], garbageBuffers[0], gameglobals.cubeVertexBuffer, 1, &copyInfo[0]);
            vkCmdCopyBuffer(garbageCmdBuffers[0], garbageBuffers[0], gameglobals.cubeIndexBuffer, 1, &copyInfo[1]);

            copyBufferToImage(garbageCmdBuffers[0], garbageBuffers[0], sizeof(vertexbuf) + sizeof(indexbuf), gameglobals.gameTextures, wallpaperX, wallpaperY, 3, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

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

        vkCmdPipelineBarrier(garbageCmdBuffers[0], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &imageBarrier);
    }

    {
        VK_ASSERT(vkEndCommandBuffer(garbageCmdBuffers[0]), "failed to end command buffer\n");

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &garbageCmdBuffers[0];

        VK_ASSERT(vkQueueSubmit(vkglobals.queue, 1, &submitInfo, garbageFences[0]), "failed to submit command buffer\n");
    }

    createImageView(&gameglobals.depthTextureView, gameglobals.depthTexture, VK_IMAGE_VIEW_TYPE_2D, gameglobals.depthTextureFormat, 1, 0, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
    createImageView(&gameglobals.gameTexturesView, gameglobals.gameTextures, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_FORMAT_R8G8B8A8_UNORM, 3, 0, VK_IMAGE_ASPECT_COLOR_BIT);
    createSampler(&gameglobals.gameTexturesSampler);

    {
        createBuffer(&gameglobals.cubeUniformBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(mat4) * 3);

        VkMemoryRequirements uboMemReq;
        vkGetBufferMemoryRequirements(vkglobals.device, gameglobals.cubeUniformBuffer, &uboMemReq);

        allocateMemory(&gameglobals.cubeBuffersMemory, uboMemReq.size, getMemoryTypeIndex(uboMemReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));

        VK_ASSERT(vkBindBufferMemory(vkglobals.device, gameglobals.cubeUniformBuffer, gameglobals.cubeBuffersMemory, 0), "failed to bind buffer memory\n");

        VK_ASSERT(vkMapMemory(vkglobals.device, gameglobals.cubeBuffersMemory, 0, VK_WHOLE_SIZE, 0, &gameglobals.cubeBuffersMemoryRaw), "failed to map memory\n");
    }

    {
        VkDescriptorPoolSize uboPoolSize = {};
        uboPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboPoolSize.descriptorCount = 1;

        VkDescriptorPoolSize textureArrayPoolSize = {};
        textureArrayPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textureArrayPoolSize.descriptorCount = 1;

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 2;
        poolInfo.pPoolSizes = (VkDescriptorPoolSize[]){uboPoolSize, textureArrayPoolSize};
        poolInfo.maxSets = 1;

        VK_ASSERT(vkCreateDescriptorPool(vkglobals.device, &poolInfo, VK_NULL_HANDLE, &gameglobals.descriptorPool), "failed to create descriptor pool\n");
    }

    {
        VkDescriptorSetLayoutBinding uboBinding = {};
        uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboBinding.binding = 0;
        uboBinding.descriptorCount = 1;
        uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

        VkDescriptorSetLayoutBinding textureArrayBinding = {};
        textureArrayBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        textureArrayBinding.binding = 1;
        textureArrayBinding.descriptorCount = 1;
        textureArrayBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = 2;
        descriptorSetLayoutInfo.pBindings = (VkDescriptorSetLayoutBinding[]){uboBinding, textureArrayBinding};

        VK_ASSERT(vkCreateDescriptorSetLayout(vkglobals.device, &descriptorSetLayoutInfo, VK_NULL_HANDLE, &gameglobals.descriptorSetLayout), "failed to create descriptor set layout\n");
    }

    {
        VkDescriptorSetAllocateInfo descriptorSetsInfo = {};
        descriptorSetsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetsInfo.descriptorPool = gameglobals.descriptorPool;
        descriptorSetsInfo.descriptorSetCount = 1;
        descriptorSetsInfo.pSetLayouts = (VkDescriptorSetLayout[]){gameglobals.descriptorSetLayout};

        VK_ASSERT(vkAllocateDescriptorSets(vkglobals.device, &descriptorSetsInfo, &gameglobals.descriptorSet), "failed to allocate descriptor sets\n");

        VkDescriptorBufferInfo descriptorBufferInfo = {};
        descriptorBufferInfo.buffer = gameglobals.cubeUniformBuffer;
        descriptorBufferInfo.offset = 0;
        descriptorBufferInfo.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet uboDescriptorWrite = {};
        uboDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uboDescriptorWrite.descriptorCount = 1;
        uboDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboDescriptorWrite.dstBinding = 0;
        uboDescriptorWrite.dstArrayElement = 0;
        uboDescriptorWrite.dstSet = gameglobals.descriptorSet;
        uboDescriptorWrite.pBufferInfo = &descriptorBufferInfo;

        VkDescriptorImageInfo descriptorImageInfo = {};
        descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfo.imageView = gameglobals.gameTexturesView;
        descriptorImageInfo.sampler = gameglobals.gameTexturesSampler;

        VkWriteDescriptorSet textureArrayDescriptorWrite = {};
        textureArrayDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        textureArrayDescriptorWrite.descriptorCount = 1;
        textureArrayDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textureArrayDescriptorWrite.dstBinding = 1;
        textureArrayDescriptorWrite.dstArrayElement = 0;
        textureArrayDescriptorWrite.dstSet = gameglobals.descriptorSet;
        textureArrayDescriptorWrite.pImageInfo = &descriptorImageInfo;

        vkUpdateDescriptorSets(vkglobals.device, 2, (VkWriteDescriptorSet[]){uboDescriptorWrite, textureArrayDescriptorWrite}, 0, VK_NULL_HANDLE);
    }

    {
        VkPushConstantRange pcRange = {};
        pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pcRange.size = sizeof(u32);
        pcRange.offset = 0;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &gameglobals.descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pcRange;

        VK_ASSERT(vkCreatePipelineLayout(vkglobals.device, &pipelineLayoutInfo, VK_NULL_HANDLE, &gameglobals.cubePipelineLayout), "failed to create pipeline layout\n");

        graphics_pipeline_info_t pipelineInfo = {};
        pipelineFillDefaultGraphicsPipeline(&pipelineInfo);
        pipelineInfo.stageCount = 2;
        pipelineInfo.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        pipelineInfo.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineInfo.stages[0].module = createShaderModuleFromFile("assets/shaders/cube.vert.spv");
        pipelineInfo.stages[1].module = createShaderModuleFromFile("assets/shaders/cube.frag.spv");
        pipelineInfo.renderingInfo.colorAttachmentCount = 1;
        pipelineInfo.renderingInfo.pColorAttachmentFormats = (VkFormat[]){vkglobals.surfaceFormat.format};
        pipelineInfo.renderingInfo.depthAttachmentFormat = gameglobals.depthTextureFormat;
        pipelineInfo.layout = gameglobals.cubePipelineLayout;

        VkVertexInputBindingDescription bindingDescs[1] = {};
        bindingDescs[0].binding = 0;
        bindingDescs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDescs[0].stride = sizeof(vec3);

        VkVertexInputAttributeDescription attributeDescs[1] = {};
        attributeDescs[0].binding = 0;
        attributeDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescs[0].location = 0;
        attributeDescs[0].offset = 0;

        pipelineInfo.vertexInputState.vertexAttributeDescriptionCount = 1;
        pipelineInfo.vertexInputState.pVertexAttributeDescriptions = attributeDescs;
        pipelineInfo.vertexInputState.vertexBindingDescriptionCount = 1;
        pipelineInfo.vertexInputState.pVertexBindingDescriptions = bindingDescs;

        pipelineInfo.depthStencilState.depthTestEnable = VK_TRUE;
        pipelineInfo.depthStencilState.depthWriteEnable = VK_TRUE;

        pipelineCreateGraphicsPipelines(VK_NULL_HANDLE, 1, &pipelineInfo, &gameglobals.cubePipeline);

        vkDestroyShaderModule(vkglobals.device, pipelineInfo.stages[0].module, VK_NULL_HANDLE);
        vkDestroyShaderModule(vkglobals.device, pipelineInfo.stages[1].module, VK_NULL_HANDLE);
    }

    {
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VK_ASSERT(vkCreateSemaphore(vkglobals.device, &semaphoreInfo, VK_NULL_HANDLE, &gameglobals.renderingDoneSemaphore), "failed to create semaphore\n");

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VK_ASSERT(vkCreateFence(vkglobals.device, &fenceInfo, VK_NULL_HANDLE, &gameglobals.frameFence), "failed to create fence\n");

        fenceInfo.flags = 0;

        VK_ASSERT(vkCreateFence(vkglobals.device, &fenceInfo, VK_NULL_HANDLE, &gameglobals.swapchainReadyFence), "failed to create fence\n");
    }

    glm_perspective(glm_rad(80.0f), (f32)vkglobals.swapchainExtent.width / vkglobals.swapchainExtent.height, 50.0f, 0.01f, gameglobals.cubeBuffersMemoryRaw + sizeof(mat4) * 2);

    {
        offset_size_t aligned = getAlignedOffsetAndSize(sizeof(mat4) * 2, sizeof(mat4), vkglobals.deviceProperties.limits.nonCoherentAtomSize);

        VkMappedMemoryRange memoryRange = {};
        memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        memoryRange.offset = aligned.offset;
        memoryRange.size = (aligned.size + aligned.offset) > sizeof(mat4) * 3 ? VK_WHOLE_SIZE : aligned.size;
        memoryRange.memory = gameglobals.cubeBuffersMemory;

        VK_ASSERT(vkFlushMappedMemoryRanges(vkglobals.device, 1, &memoryRange), "failed to flush device memory\n");
    }

    gameglobals.loopActive = 1;
    gameglobals.deltaTime = 0;
    gameglobals.shift = 1;

    garbageWaitAndDestroy(GARBAGE_CMD_BUFFER_NUM, garbageCmdBuffers, GARBAGE_BUFFER_NUM, garbageBuffers, GARBAGE_BUFFER_MEMORY_NUM, garbageBuffersMem, GARBAGE_FENCE_NUM, garbageFences);
}

void gameEvent(SDL_Event* e) {
    if (e->type == SDL_EVENT_KEY_DOWN) {
        if (e->key.key == SDLK_W) gameglobals.input[0] = 1.0f / 2000.0f;
        if (e->key.key == SDLK_S) gameglobals.input[1] = -1.0f / 2000.0f;
        if (e->key.key == SDLK_A) gameglobals.input[2] = -1.0f / 2000.0f;
        if (e->key.key == SDLK_D) gameglobals.input[3] = 1.0f / 2000.0f;
        if (e->key.key == SDLK_LSHIFT) gameglobals.shift = 3;
    } else if (e->type == SDL_EVENT_KEY_UP) {
        if (e->key.key == SDLK_W) gameglobals.input[0] = 0;
        if (e->key.key == SDLK_S) gameglobals.input[1] = 0;
        if (e->key.key == SDLK_A) gameglobals.input[2] = 0;
        if (e->key.key == SDLK_D) gameglobals.input[3] = 0;
        if (e->key.key == SDLK_LSHIFT) gameglobals.shift = 1;
    } else if (e->type == SDL_EVENT_MOUSE_MOTION) {
        gameglobals.cam.yaw += (f32)e->motion.xrel / 400.0f;
        gameglobals.cam.pitch -= (f32)e->motion.yrel / 400.0f;
        clampf(&gameglobals.cam.pitch, -M_PI / 2.0f, M_PI / 2.0f);
    }
}

void updateCubeUbo() {
    versor y, p;
    glm_quatv(y, gameglobals.cam.yaw, (vec3){0.0f, 1.0f, 0.0f});
    glm_quatv(p, gameglobals.cam.pitch, (vec3){1.0f, 0.0f, 0.0f});
    
    {
        mat4 rot;
        glm_quat_mat4(y, rot);
        vec3 vel;
        glm_mat4_mulv3(rot, (vec3){(gameglobals.input[2] + gameglobals.input[3]) * gameglobals.shift * gameglobals.deltaTime, 0.0f, (gameglobals.input[0] + gameglobals.input[1]) * gameglobals.shift * gameglobals.deltaTime}, 0.0f, vel);
        glm_vec3_add(gameglobals.cam.position, vel, gameglobals.cam.position);

        clampf(&gameglobals.cam.position[0], -0.9f, 0.9f);
        clampf(&gameglobals.cam.position[1], -0.9f, 0.9f);
        clampf(&gameglobals.cam.position[2], -0.9f, 0.9f);
    }

    {
        glm_quat_mul(y, p, y);
        glm_quat_look(gameglobals.cam.position, y, gameglobals.cubeBuffersMemoryRaw + sizeof(mat4));
        glm_translate(gameglobals.cubeBuffersMemoryRaw + sizeof(mat4), (vec3){-gameglobals.cam.position[0], -gameglobals.cam.position[1], -gameglobals.cam.position[2]});
    }

    glm_mat4_identity(gameglobals.cubeBuffersMemoryRaw);

    VkDeviceSize alignedSize = sizeof(mat4) * 2 + getAlignCooficient(sizeof(mat4) * 2, vkglobals.deviceProperties.limits.nonCoherentAtomSize);
    VkMappedMemoryRange memoryRange = {};
    memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memoryRange.offset = 0;
    memoryRange.size = alignedSize > sizeof(mat4) * 3 ? VK_WHOLE_SIZE : alignedSize;
    memoryRange.memory = gameglobals.cubeBuffersMemory;

    VK_ASSERT(vkFlushMappedMemoryRanges(vkglobals.device, 1, &memoryRange), "failed to flush device memory\n");
}

void gameRender() {
    updateCubeUbo();

    u32 imageIndex;
    VK_ASSERT(vkAcquireNextImageKHR(vkglobals.device, vkglobals.swapchain, 0xFFFFFFFFFFFFFFFF, VK_NULL_HANDLE, gameglobals.swapchainReadyFence, &imageIndex), "failed to acquire swapchain image\n");
    VK_ASSERT(vkWaitForFences(vkglobals.device, 2, (VkFence[]){gameglobals.swapchainReadyFence, gameglobals.frameFence}, VK_TRUE, 0xFFFFFFFFFFFFFFFF), "failed to wait for fences\n");
    VK_ASSERT(vkResetFences(vkglobals.device, 2, (VkFence[]){gameglobals.swapchainReadyFence, gameglobals.frameFence}), "failed to reset fences\n");

    {
        VkCommandBufferBeginInfo cmdBeginInfo = {};
        cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_ASSERT(vkBeginCommandBuffer(vkglobals.cmdBuffer, &cmdBeginInfo), "failed to begin command buffer\n");

        {
            VkImageMemoryBarrier imageBarrier = {};
            imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarrier.image = vkglobals.swapchainImages[imageIndex];
            imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarrier.subresourceRange.baseArrayLayer = 0;
            imageBarrier.subresourceRange.layerCount = 1;
            imageBarrier.subresourceRange.baseMipLevel = 0;
            imageBarrier.subresourceRange.levelCount = 1;
            imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarrier.srcAccessMask = 0;
            imageBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            vkCmdPipelineBarrier(vkglobals.cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &imageBarrier);
        }

        VkRenderingAttachmentInfoKHR swapchainImageAttachment = {};
        swapchainImageAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        swapchainImageAttachment.imageView = vkglobals.swapchainImageViews[imageIndex];
        swapchainImageAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        swapchainImageAttachment.resolveMode = VK_RESOLVE_MODE_NONE_KHR;
        swapchainImageAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        swapchainImageAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        swapchainImageAttachment.clearValue = (VkClearValue){{{0.0f, 0.0f, 0.0f, 0.0f}}};

        VkRenderingAttachmentInfoKHR depthTextureAttachment = {};
        depthTextureAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        depthTextureAttachment.imageView = gameglobals.depthTextureView;
        depthTextureAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthTextureAttachment.resolveMode = VK_RESOLVE_MODE_NONE_KHR;
        depthTextureAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthTextureAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthTextureAttachment.clearValue = (VkClearValue){{{0.0f, 0}}};

        VkRenderingInfoKHR renderingInfo = {};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
        renderingInfo.renderArea = (VkRect2D){(VkOffset2D){0, 0}, vkglobals.swapchainExtent};
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &swapchainImageAttachment;
        renderingInfo.pDepthAttachment = &depthTextureAttachment;
        renderingInfo.pStencilAttachment = VK_NULL_HANDLE;

        vkCmdBeginRenderingKHR(vkglobals.cmdBuffer, &renderingInfo);

        VkDeviceSize vertexBufferOffsets[1] = {};
        vkCmdBindPipeline(vkglobals.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gameglobals.cubePipeline);
        vkCmdBindVertexBuffers(vkglobals.cmdBuffer, 0, 1, &gameglobals.cubeVertexBuffer, vertexBufferOffsets);
        vkCmdBindIndexBuffer(vkglobals.cmdBuffer, gameglobals.cubeIndexBuffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(vkglobals.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gameglobals.cubePipelineLayout, 0, 1, &gameglobals.descriptorSet, 0, VK_NULL_HANDLE);

        u32 textureArrayIndices[4] = {1, 2, 0 | (1 << 2), 0 | (2 << 2)};

        vkCmdBindIndexBuffer(vkglobals.cmdBuffer, gameglobals.cubeIndexBuffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdPushConstants(vkglobals.cmdBuffer, gameglobals.cubePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(u32), &textureArrayIndices[0]);
        vkCmdDrawIndexed(vkglobals.cmdBuffer, 6, 1, 0, 0, 0);

        vkCmdBindIndexBuffer(vkglobals.cmdBuffer, gameglobals.cubeIndexBuffer, 6 * sizeof(u16), VK_INDEX_TYPE_UINT16);
        vkCmdPushConstants(vkglobals.cmdBuffer, gameglobals.cubePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(u32), &textureArrayIndices[1]);
        vkCmdDrawIndexed(vkglobals.cmdBuffer, 6, 1, 0, 0, 0);

        vkCmdBindIndexBuffer(vkglobals.cmdBuffer, gameglobals.cubeIndexBuffer, 12 * sizeof(u16), VK_INDEX_TYPE_UINT16);
        vkCmdPushConstants(vkglobals.cmdBuffer, gameglobals.cubePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(u32), &textureArrayIndices[2]);
        vkCmdDrawIndexed(vkglobals.cmdBuffer, 12, 1, 0, 0, 0);

        vkCmdBindIndexBuffer(vkglobals.cmdBuffer, gameglobals.cubeIndexBuffer, 24 * sizeof(u16), VK_INDEX_TYPE_UINT16);
        vkCmdPushConstants(vkglobals.cmdBuffer, gameglobals.cubePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(u32), &textureArrayIndices[3]);
        vkCmdDrawIndexed(vkglobals.cmdBuffer, 12, 1, 0, 0, 0);

        vkCmdEndRenderingKHR(vkglobals.cmdBuffer);

        {
            VkImageMemoryBarrier imageBarrier = {};
            imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarrier.image = vkglobals.swapchainImages[imageIndex];
            imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarrier.subresourceRange.baseArrayLayer = 0;
            imageBarrier.subresourceRange.layerCount = 1;
            imageBarrier.subresourceRange.baseMipLevel = 0;
            imageBarrier.subresourceRange.levelCount = 1;
            imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            imageBarrier.dstAccessMask = 0;
            imageBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            imageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            vkCmdPipelineBarrier(vkglobals.cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &imageBarrier);
        }

        VK_ASSERT(vkEndCommandBuffer(vkglobals.cmdBuffer), "failed to end command buffer\n");
    }

    VkPipelineStageFlags semaphoreSignalStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = VK_NULL_HANDLE;
    submitInfo.pWaitDstStageMask = &semaphoreSignalStage;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &gameglobals.renderingDoneSemaphore;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkglobals.cmdBuffer;

    VK_ASSERT(vkQueueSubmit(vkglobals.queue, 1, &submitInfo, gameglobals.frameFence), "failed to submit command buffer\n");

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &vkglobals.swapchain;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &gameglobals.renderingDoneSemaphore;
    presentInfo.pImageIndices = &imageIndex;

    VK_ASSERT(vkQueuePresentKHR(vkglobals.queue, &presentInfo), "failed to present swapchain image\n");
}

void gameQuit() {
    vkDestroyFence(vkglobals.device, gameglobals.frameFence, VK_NULL_HANDLE);
    vkDestroyFence(vkglobals.device, gameglobals.swapchainReadyFence, VK_NULL_HANDLE);
    vkDestroySemaphore(vkglobals.device, gameglobals.renderingDoneSemaphore, VK_NULL_HANDLE);
    vkDestroyPipeline(vkglobals.device, gameglobals.cubePipeline, VK_NULL_HANDLE);
    vkDestroyPipelineLayout(vkglobals.device, gameglobals.cubePipelineLayout, VK_NULL_HANDLE);
    vkUnmapMemory(vkglobals.device, gameglobals.cubeBuffersMemory);
    vkDestroyBuffer(vkglobals.device, gameglobals.cubeUniformBuffer, VK_NULL_HANDLE);
    vkFreeMemory(vkglobals.device, gameglobals.cubeBuffersMemory, VK_NULL_HANDLE);
    vkDestroyBuffer(vkglobals.device, gameglobals.cubeIndexBuffer, VK_NULL_HANDLE);
    vkDestroyBuffer(vkglobals.device, gameglobals.cubeVertexBuffer, VK_NULL_HANDLE);
    vkDestroySampler(vkglobals.device, gameglobals.gameTexturesSampler, VK_NULL_HANDLE);
    vkDestroyImageView(vkglobals.device, gameglobals.gameTexturesView, VK_NULL_HANDLE);
    vkDestroyImage(vkglobals.device, gameglobals.gameTextures, VK_NULL_HANDLE);
    vkDestroyImageView(vkglobals.device, gameglobals.depthTextureView, VK_NULL_HANDLE);
    vkDestroyImage(vkglobals.device, gameglobals.depthTexture, VK_NULL_HANDLE);
    vkFreeMemory(vkglobals.device, gameglobals.deviceLocalMemory, VK_NULL_HANDLE);
    vkDestroyDescriptorSetLayout(vkglobals.device, gameglobals.descriptorSetLayout, VK_NULL_HANDLE);
    vkDestroyDescriptorPool(vkglobals.device, gameglobals.descriptorPool, VK_NULL_HANDLE);
}