#ifndef GAME_H
#define GAME_H

#include <vulkan/vulkan.h>
#include <cglm/cglm.h>
#include <SDL3/SDL.h>

#include "numtypes.h"

typedef struct {
    u8 loopActive;
    u32 deltaTime;

    struct {
        vec3 position;
        f32 pitch, yaw;
    } cam;

    vec4 input;
    u8 shift;
    VkFormat depthTextureFormat;

    VkSampler sampler;

    // device local memory
    VkDeviceMemory deviceLocalDepthStencilAttachmentMemory;
    VkDeviceMemory deviceLocalColorAttachmentSampledMemory;
    VkDeviceMemory deviceLocalSampledTransferDstMemory;
    VkDeviceMemory deviceLocalUniformTransferDstMemory;
    VkDeviceMemory deviceLocalVertexTransferDstMemory;

    // host visible memory
    VkDeviceMemory hostVisibleUniformMemory;

    void* hostVisibleUniformMemoryRaw;


    // device local resources
    VkBuffer deviceLocalUniformBuffer;
    VkBuffer cubeVertexBuffer;

    VkImage depthTexture;
    VkImage cubeTextures;
    VkImage gbuffer;
    VkImage ssaoNoiseTexture;
    VkImage ssaoAttachment;

    VkImageView depthTextureView;
    VkImageView cubeTexturesView;
    VkImageView gbufferAlbedoView;
    VkImageView gbufferPositionView;
    VkImageView gbufferNormalView;
    VkImageView ssaoNoiseTextureView;
    VkImageView ssaoAttachmentView;
    VkImageView ssaoBlurAttachmentView;

    VkDeviceSize ssaoNoiseTextureOffset;

    // host visible resources
    VkBuffer cubeUniformBuffer;


    VkDescriptorPool descriptorPool;

    VkDescriptorSetLayout projectionMatrixdescriptorSetLayout;
    VkDescriptorSetLayout cubeDescriptorSetLayout;
    VkDescriptorSetLayout ssaoDataDescriptorSetLayout;
    VkDescriptorSetLayout gbufferDescriptorSetLayout;
    VkDescriptorSetLayout sampledImageDescriptorSetLayout;
    
    VkDescriptorSet projectionMatrixdescriptorSet;
    VkDescriptorSet cubeDescriptorSet;
    VkDescriptorSet ssaoDataDescriptorSet;
    VkDescriptorSet gbufferDescriptorSet;
    VkDescriptorSet ssaoAttachmentDescriptorSet;
    VkDescriptorSet ssaoBlurAttachmentDescriptorSet;


    VkPipelineLayout sampledImagePipelineLayout;
    VkPipelineLayout cubePipelineLayout;
    VkPipelineLayout ssaoPipelineLayout;
    VkPipelineLayout compositionPipelineLayout;

    VkPipeline cubePipeline;
    VkPipeline ssaoPipeline;
    VkPipeline ssaoBlurPipeline;
    VkPipeline compositionPipeline;

    VkSemaphore renderingDoneSemaphore;

    VkFence swapchainReadyFence;
    VkFence frameFence;
} game_globals_t;

void gameInit();
void gameEvent(SDL_Event* e);
void gameRender();
void gameQuit();

extern game_globals_t gameglobals;

#endif