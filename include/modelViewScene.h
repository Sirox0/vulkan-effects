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

    struct {
        vec3 position;
        f32 pitch, yaw;
        f32 targetPitch, targetYaw;
    } cam;

    vec2 inputX;
    vec2 inputY;
    vec2 inputZ;
    f32 shift;
    VkFormat depthTextureFormat;

    VkSampler sampler;

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
    VkBuffer projectionMatrixBuffer;
    VkBuffer ssaoKernelBuffer;
    VkBuffer skyboxVertexBuffer;

    VkImage depthTexture;
    VkImage skyboxCubemap;
    VkImage gbufferPositionVelocity;
    VkImage gbufferNormalAlbedo;
    VkImage ssaoNoiseTexture;
    VkImage ssaoAttachment;
    VkImage postProcessAttachment;

    VkImageView depthTextureView;
    VkImageView skyboxCubemapView;
    VkImageView gbufferPositionView;
    VkImageView gbufferVelocityView;
    VkImageView gbufferNormalView;
    VkImageView gbufferAlbedoView;
    VkImageView ssaoNoiseTextureView;
    VkImageView ssaoAttachmentView;
    VkImageView ssaoBlurAttachmentView;
    VkImageView postProcessAttachmentView;

    // host visible resources
    VkBuffer viewMatrixBuffer;


    VkDescriptorPool descriptorPool;

    VkDescriptorSetLayout ssaoDataDescriptorSetLayout;
    VkDescriptorSetLayout gbufferDescriptorSetLayout;
    VkDescriptorSetLayout modelDescriptorSetLayout;

    VkDescriptorSetLayout PVmatrixdescriptorSetLayout;
    VkDescriptorSetLayout uniformDescriptorSetLayout;
    VkDescriptorSetLayout sampledImageDescriptorSetLayout;
    VkDescriptorSetLayout combinedImageSamplerDescriptorSetLayout;
    
    VkDescriptorSet PVmatrixdescriptorSet;
    VkDescriptorSet skyboxCubemapDescriptorSet;
    VkDescriptorSet ssaoDataDescriptorSet;
    VkDescriptorSet gbufferDescriptorSet;
    VkDescriptorSet ssaoAttachmentDescriptorSet;
    VkDescriptorSet ssaoBlurAttachmentDescriptorSet;
    VkDescriptorSet postProcessAttachmentDescriptorSet;


    VkPipelineLayout sampledImagePipelineLayout;
    VkPipelineLayout modelPipelineLayout;
    VkPipelineLayout skyboxPipelineLayout;
    VkPipelineLayout ssaoPipelineLayout;
    VkPipelineLayout compositionPipelineLayout;
    VkPipelineLayout uberPipelineLayout;

    VkPipeline modelPipeline;
    VkPipeline skyboxPipeline;
    VkPipeline ssaoPipeline;
    VkPipeline ssaoBlurPipeline;
    VkPipeline compositionPipeline;
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