#ifndef PIPELINE_H
#define PIPELINE_H

#include <vulkan/vulkan.h>

#include "numtypes.h"

typedef struct {
    void* pNext;
    VkPipelineRenderingCreateInfo renderingInfo;
    VkPipelineCreateFlags flags;
    u32 stageCount;
    VkPipelineShaderStageCreateInfo stages[5];
    VkPipelineVertexInputStateCreateInfo vertexInputState;
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
    VkPipelineTessellationStateCreateInfo tessellationState;
    VkViewport viewport;
    VkRect2D scissor;
    VkPipelineViewportStateCreateInfo viewportState;
    VkPipelineRasterizationStateCreateInfo rasterizationState;
    VkPipelineMultisampleStateCreateInfo multisampleState;
    VkPipelineDepthStencilStateCreateInfo depthStencilState;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo colorBlendState;
    VkPipelineDynamicStateCreateInfo dynamicState;
    VkPipelineLayout layout;
    VkPipeline basePipelineHandle;
    i32 basePipelineIndex;
} VkGraphicsPipelineInfo_t;

typedef struct {
    void* pNext;
    VkPipelineCreateFlags flags;
    VkPipelineShaderStageCreateInfo stage;
    VkPipelineLayout layout;
    VkPipeline basePipelineHandle;
    i32 basePipelineIndex;
} VkComputePipelineInfo_t;

void pipelineFillDefaultGraphicsPipeline(VkGraphicsPipelineInfo_t* pInfo);
void pipelineFillDefaultComputePipeline(VkComputePipelineInfo_t* pInfo);
void pipelineCreateComputePipelines(VkPipelineCache cache, u32 infoCount, VkComputePipelineInfo_t* pInfos, VkPipeline* pPipelines);
void pipelineCreateGraphicsPipelines(VkPipelineCache cache, u32 infoCount, VkGraphicsPipelineInfo_t* pInfos, VkPipeline* pPipelines);

#endif