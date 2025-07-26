#ifndef GAME_H
#define GAME_H

#include <vulkan/vulkan.h>
#include <cglm/cglm.h>
#include <SDL3/SDL.h>

#include "numtypes.h"
#include "vkModel.h"

typedef struct {
    u32 swapchainImageCount;
    VkImage* swapchainImages;
    VkImageView* swapchainImageViews;

    VkCommandBuffer cmdBuffer;

    VkPipelineCache pipelineCache;

    struct {
        vec3 position;
        f32 pitch, yaw;
        f32 targetPitch, targetYaw;
    } cam;

    vec4 lightPos;

    vec2 inputX;
    vec2 inputY;
    vec2 inputZ;
    f32 shift;
    VkFormat depthFormat;

    VkSampler sampler;
    VkSampler shadowmapSampler;

    // device local memory
    VkDeviceMemory deviceLocalDepthStencilAttachmentMemory;
    VkDeviceMemory deviceLocalColorAttachmentSampledMemory;
    VkDeviceMemory deviceLocalSampledTransferDstMemory;
    VkDeviceMemory deviceLocalUniformTransferDstMemory;
    VkDeviceMemory deviceLocalStorageTransferDstMemory;
    VkDeviceMemory deviceLocalVertexTransferDstMemory;
    VkDeviceMemory deviceLocalIndexTransferDstMemory;
    VkDeviceMemory deviceLocalIndirectTransferDstMemory;

    // host visible memory
    VkDeviceMemory hostVisibleUniformMemory;
    VkDeviceMemory hostVisibleStorageMemory;

    void* hostVisibleUniformMemoryRaw;
    void* hostVisibleStorageMemoryRaw;


    // device local resources
    VkBuffer projectionBuffer;
    VkBuffer skyboxVertexBuffer;

    VkImage depthTexture;
    VkImage gbuffer;
    VkImage metallicRoughnessVelocityTexture;
    VkImage ssaoAttachment;
    VkImage postProcessAttachment;
    VkImage blurredPostProcessAttachment;
    VkImage shadowmap;

    VkImageView depthTextureView;
    VkImageView gbufferNormalView;
    VkImageView gbufferAlbedoView;
    VkImageView metallicRoughnessVelocityTextureView;
    VkImageView ssaoAttachmentView;
    VkImageView postProcessAttachmentView;
    VkImageView blurredPostProcessAttachmentView;
    VkImageView shadowmapView;

    // host visible resources
    VkBuffer viewMatrixBuffer;
    VkBuffer lightBuffer;

    VkDeviceSize lightBufferOffset;


    VkDescriptorPool descriptorPool;

    VkDescriptorSetLayout storageDescriptorSetLayout;
    VkDescriptorSetLayout gbufferDescriptorSetLayout;
    VkDescriptorSetLayout modelDescriptorSetLayout;

    VkDescriptorSetLayout PVmatrixdescriptorSetLayout;
    VkDescriptorSetLayout uniformDescriptorSetLayout;
    VkDescriptorSetLayout sampledImageDescriptorSetLayout;
    VkDescriptorSetLayout combinedImageSamplerDescriptorSetLayout;
    
    VkDescriptorSet lightDescriptorSet;
    VkDescriptorSet PVmatrixdescriptorSet;
    VkDescriptorSet gbufferDescriptorSet;
    VkDescriptorSet ssaoAttachmentDescriptorSet;
    VkDescriptorSet postProcessAttachmentDescriptorSet;
    VkDescriptorSet blurredPostProcessAttachmentDescriptorSet;
    VkDescriptorSet shadowmapDescriptorSet;


    VkPipelineLayout sampledImagePipelineLayout;
    VkPipelineLayout shadowmapPipelineLayout;
    VkPipelineLayout modelPipelineLayout;
    VkPipelineLayout PVPipelineLayout;
    VkPipelineLayout PVgbufferPipelineLayout;
    VkPipelineLayout compositionPipelineLayout;
    VkPipelineLayout uberPipelineLayout;

    VkPipeline shadowmapPipeline;
    VkPipeline modelPipeline;
    VkPipeline skyboxPipeline;
    VkPipeline ssaoPipeline;
    VkPipeline compositionPipeline;
    VkPipeline gaussianBlurPipeline;
    VkPipeline uberPipeline;

    VkSemaphore* renderingDoneSemaphores;
    VkSemaphore swapchainReadySemaphore;
    
    VkFence frameFence;

    VkModel_t model;
} ModelViewSceneGlobals_t;

void modelViewSceneInit();
void modelViewSceneEvent(SDL_Event* e);
void modelViewSceneRender();
void modelViewSceneQuit();

#endif