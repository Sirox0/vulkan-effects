#ifndef GAME_H
#define GAME_H

#include <vulkan/vulkan.h>
#include <cglm/cglm.h>
#include <SDL3/SDL.h>

#include "numtypes.h"

#define FT_ASSERT(expression) \
    { \
        FT_Error error = expression; \
        if (error != 0) { \
            printf("freetype error: %s\n", FT_Error_String(error)); \
            exit(1); \
        } \
    }

typedef struct {
    u8 loopActive;
    u32 deltaTime;

    VkDescriptorPool descriptorPool;

    VkSampler sampler;

    // layout: depth texture -> game textures -> cube vertex buffer
    VkDeviceMemory deviceLocalMemory;

    VkFormat depthTextureFormat;
    VkImage depthTexture;
    VkImageView depthTextureView;

    VkImage gameTextures;
    VkDeviceSize gameTexturesOffset;
    VkImageView gameTexturesView;

    VkImage gbuffer;
    VkDeviceSize gbufferOffset;
    VkImageView gbufferAlbedoView;
    VkImageView gbufferPositionView;
    VkImageView gbufferNormalView;

    VkBuffer cubeVertexBuffer;
    VkDeviceSize cubeVertexBufferOffset;

    VkBuffer cubeUniformBuffer;
    VkDeviceMemory cubeBuffersMemory;
    void* cubeBuffersMemoryRaw;

    VkDescriptorSetLayout cubeDescriptorSetLayout;
    VkDescriptorSet cubeDescriptorSet;

    VkPipelineLayout cubePipelineLayout;
    VkPipeline cubePipeline;

    VkDescriptorSetLayout compositionDescriptorSetLayout;
    VkDescriptorSet compositionDescriptorSet;

    VkPipelineLayout compositionPipelineLayout;
    VkPipeline compositionPipeline;

    struct {
        vec3 position;
        f32 pitch, yaw;
    } cam;

    vec4 input;
    u8 shift;

    VkSemaphore renderingDoneSemaphore;
    VkFence swapchainReadyFence;
    VkFence frameFence;
} game_globals_t;

#define MAX_STAR_SCALE 1.1f
#define LERP_STAR_INVERSE_SPEED 100.0f

void gameInit();
void gameEvent(SDL_Event* e);
void gameRender();
void gameQuit();

extern game_globals_t gameglobals;

#endif