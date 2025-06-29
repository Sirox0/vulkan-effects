#include <vulkan/vulkan.h>

#include "numtypes.h"
#include "vkFunctions.h"
#include "vk.h"
#include "pipeline.h"

void pipelineFillDefaultGraphicsPipeline(VkGraphicsPipelineInfo_t* pInfo) {
    pInfo->pNext = VK_NULL_HANDLE;
    pInfo->flags = 0;
    pInfo->stageCount = 0; // must be set by user
    for (u32 i = 0; i < 5; i++) {
        pInfo->stages[i].pNext = VK_NULL_HANDLE;
        pInfo->stages[i].flags = 0;
        pInfo->stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pInfo->stages[i].stage = VK_SHADER_STAGE_ALL; // must be set by user
        pInfo->stages[i].pName = "main";
        pInfo->stages[i].module = VK_NULL_HANDLE; // must be set by user
        pInfo->stages[i].pSpecializationInfo = VK_NULL_HANDLE;
    }

    pInfo->vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pInfo->vertexInputState.pNext = VK_NULL_HANDLE;
    pInfo->vertexInputState.flags = 0;
    pInfo->vertexInputState.vertexAttributeDescriptionCount = 0;
    pInfo->vertexInputState.pVertexAttributeDescriptions = VK_NULL_HANDLE;
    pInfo->vertexInputState.vertexBindingDescriptionCount = 0;
    pInfo->vertexInputState.pVertexBindingDescriptions = VK_NULL_HANDLE;

    pInfo->inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pInfo->inputAssemblyState.pNext = VK_NULL_HANDLE;
    pInfo->inputAssemblyState.flags = 0;
    pInfo->inputAssemblyState.primitiveRestartEnable = VK_FALSE;
    pInfo->inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    pInfo->tessellationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    pInfo->tessellationState.pNext = VK_NULL_HANDLE;
    pInfo->tessellationState.flags = 0;
    pInfo->tessellationState.patchControlPoints = 4;

    pInfo->viewport.x = 0;
    pInfo->viewport.y = 0;
    pInfo->viewport.minDepth = 0.0f;
    pInfo->viewport.maxDepth = 1.0f;
    pInfo->viewport.width = vkglobals.swapchainExtent.width;
    pInfo->viewport.height = vkglobals.swapchainExtent.height;

    pInfo->scissor.extent = vkglobals.swapchainExtent;
    pInfo->scissor.offset = (VkOffset2D){0, 0};

    pInfo->viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pInfo->viewportState.pNext = VK_NULL_HANDLE;
    pInfo->viewportState.flags = 0;
    pInfo->viewportState.scissorCount = 1;
    pInfo->viewportState.pScissors = &pInfo->scissor;
    pInfo->viewportState.viewportCount = 1;
    pInfo->viewportState.pViewports = &pInfo->viewport;

    pInfo->rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pInfo->rasterizationState.pNext = VK_NULL_HANDLE;
    pInfo->rasterizationState.flags = 0;
    pInfo->rasterizationState.depthClampEnable = VK_FALSE;
    pInfo->rasterizationState.rasterizerDiscardEnable = VK_FALSE;
    pInfo->rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    pInfo->rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    pInfo->rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
    pInfo->rasterizationState.depthBiasEnable = VK_FALSE;
    pInfo->rasterizationState.lineWidth = 1.0f;

    pInfo->multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pInfo->multisampleState.pNext = VK_NULL_HANDLE;
    pInfo->multisampleState.flags = 0;
    pInfo->multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pInfo->multisampleState.sampleShadingEnable = VK_FALSE;
    pInfo->multisampleState.minSampleShading = 1.0f;
    pInfo->multisampleState.pSampleMask = VK_NULL_HANDLE;
    pInfo->multisampleState.alphaToCoverageEnable = VK_FALSE;
    pInfo->multisampleState.alphaToOneEnable = VK_FALSE;

    pInfo->depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pInfo->depthStencilState.pNext = VK_NULL_HANDLE;
    pInfo->depthStencilState.flags = 0;
    pInfo->depthStencilState.depthTestEnable = VK_FALSE;
    pInfo->depthStencilState.depthWriteEnable = VK_FALSE;
    pInfo->depthStencilState.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
    pInfo->depthStencilState.depthBoundsTestEnable = VK_FALSE;
    pInfo->depthStencilState.stencilTestEnable = VK_FALSE;
    pInfo->depthStencilState.minDepthBounds = 0.0f;
    pInfo->depthStencilState.maxDepthBounds = 1.0f;

    pInfo->colorBlendAttachment.blendEnable = VK_FALSE;
    pInfo->colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    pInfo->colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    pInfo->colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    pInfo->colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    pInfo->colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_CONSTANT_ALPHA;
    pInfo->colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_CONSTANT_ALPHA;
    pInfo->colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    pInfo->colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pInfo->colorBlendState.pNext = VK_NULL_HANDLE;
    pInfo->colorBlendState.flags = 0;
    pInfo->colorBlendState.logicOpEnable = VK_FALSE;
    pInfo->colorBlendState.logicOp = VK_LOGIC_OP_COPY;
    pInfo->colorBlendState.attachmentCount = 1;
    pInfo->colorBlendState.pAttachments = &pInfo->colorBlendAttachment;
    pInfo->colorBlendState.blendConstants[0] = 0.0f;
    pInfo->colorBlendState.blendConstants[1] = 0.0f;
    pInfo->colorBlendState.blendConstants[2] = 0.0f;
    pInfo->colorBlendState.blendConstants[3] = 0.5f;

    pInfo->dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pInfo->dynamicState.pNext = VK_NULL_HANDLE;
    pInfo->dynamicState.flags = 0;
    pInfo->dynamicState.dynamicStateCount = 0;
    pInfo->dynamicState.pDynamicStates = VK_NULL_HANDLE;

    pInfo->renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    pInfo->renderingInfo.viewMask = 0;
    pInfo->renderingInfo.colorAttachmentCount = 0;
    pInfo->renderingInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    pInfo->renderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    pInfo->layout = VK_NULL_HANDLE; // must be set by user
    pInfo->basePipelineHandle = VK_NULL_HANDLE;
    pInfo->basePipelineIndex = 0;
}

void pipelineFillDefaultComputePipeline(VkComputePipelineInfo_t* pInfo) {
    pInfo->pNext = VK_NULL_HANDLE;
    pInfo->flags = 0;
    pInfo->stage.pNext = VK_NULL_HANDLE;
    pInfo->stage.flags = 0;
    pInfo->stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pInfo->stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pInfo->stage.pName = "main";
    pInfo->stage.module = VK_NULL_HANDLE; // must be set by user
    pInfo->stage.pSpecializationInfo = VK_NULL_HANDLE;
    pInfo->layout = VK_NULL_HANDLE; // must be set by user
    pInfo->basePipelineHandle = VK_NULL_HANDLE;
    pInfo->basePipelineIndex = 0;
}

void pipelineCreateComputePipelines(VkPipelineCache cache, u32 infoCount, VkComputePipelineInfo_t* pInfos, VkPipeline* pPipelines) {
    VkComputePipelineCreateInfo createInfos[infoCount];

    for (u32 i = 0; i < infoCount; i++) {
        createInfos[i].sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        createInfos[i].pNext = pInfos[i].pNext;
        createInfos[i].flags = pInfos[i].flags;
        createInfos[i].stage = pInfos[i].stage;
        createInfos[i].layout = pInfos[i].layout;
        createInfos[i].basePipelineHandle = pInfos[i].basePipelineHandle;
        createInfos[i].basePipelineIndex = pInfos[i].basePipelineIndex;
    }

    VK_ASSERT(vkCreateComputePipelines(vkglobals.device, cache, infoCount, createInfos, VK_NULL_HANDLE, pPipelines), "failed to create compute pipelines\n");
}

void pipelineCreateGraphicsPipelines(VkPipelineCache cache, u32 infoCount, VkGraphicsPipelineInfo_t* pInfos, VkPipeline* pPipelines) {
    VkGraphicsPipelineCreateInfo createInfos[infoCount];

    for (u32 i = 0; i < infoCount; i++) {
        pInfos->renderingInfo.pNext = pInfos->pNext;
        createInfos[i] = (VkGraphicsPipelineCreateInfo){};
        createInfos[i].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        createInfos[i].pNext = &pInfos[i].renderingInfo;
        createInfos[i].flags = pInfos[i].flags;
        createInfos[i].stageCount = pInfos[i].stageCount;
        createInfos[i].pStages = pInfos[i].stages;
        createInfos[i].pVertexInputState = &pInfos[i].vertexInputState;
        createInfos[i].pInputAssemblyState = &pInfos[i].inputAssemblyState;
        createInfos[i].pTessellationState = &pInfos[i].tessellationState;
        createInfos[i].pViewportState = &pInfos[i].viewportState;
        createInfos[i].pRasterizationState = &pInfos[i].rasterizationState;
        createInfos[i].pMultisampleState = &pInfos[i].multisampleState;
        createInfos[i].pRasterizationState = &pInfos[i].rasterizationState;
        createInfos[i].pDepthStencilState = &pInfos[i].depthStencilState;
        createInfos[i].pColorBlendState = &pInfos[i].colorBlendState;
        createInfos[i].pDynamicState = &pInfos[i].dynamicState;
        createInfos[i].layout = pInfos[i].layout;
        createInfos[i].basePipelineHandle = pInfos[i].basePipelineHandle;
        createInfos[i].basePipelineIndex = pInfos[i].basePipelineIndex;
    }

    VK_ASSERT(vkCreateGraphicsPipelines(vkglobals.device, cache, infoCount, createInfos, VK_NULL_HANDLE, pPipelines), "failed to create graphics pipelines\n");
}