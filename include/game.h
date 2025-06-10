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
    vec3 pos;
    vec3 color;
} vertex_input_cube_t;

typedef struct {
    u8 loopActive;
    u32 deltaTime;

    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout uboDescriptorSetLayout;

    // layout: depth texture -> cube vertex buffer -> cube index buffer
    VkDeviceMemory deviceLocalMemory;

    VkFormat depthTextureFormat;
    VkImage depthTexture;
    VkImageView depthTextureView;

    VkBuffer cubeVertexBuffer;
    VkDeviceSize cubeVertexBufferOffset;
    VkBuffer cubeIndexBuffer;
    VkDeviceSize cubeIndexBufferOffset;

    VkBuffer cubeUniformBuffer;
    VkDeviceMemory cubeBuffersMemory;
    void* cubeBuffersMemoryRaw;

    VkDescriptorSet cubeUboDescriptorSet;
    VkPipelineLayout cubePipelineLayout;
    VkPipeline cubePipeline;

    struct {
        vec3 velocity;
        vec3 position;
        f32 pitch, yaw;
    } cam;

    VkSemaphore swapchainReadySemaphore;
    VkSemaphore renderingDoneSemaphore;
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