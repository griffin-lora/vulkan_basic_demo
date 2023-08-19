#pragma once
#include "vk.h"

extern const VkPipelineInputAssemblyStateCreateInfo default_input_assembly_create_info;
extern const VkPipelineViewportStateCreateInfo default_viewport_create_info;
extern const VkPipelineDepthStencilStateCreateInfo default_depth_stencil_create_info;
extern const VkPipelineColorBlendStateCreateInfo default_color_blend_create_info;
extern const VkPipelineDynamicStateCreateInfo default_dynamic_create_info;

#define DEFAULT_VK_COMMAND_BUFFER\
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,\
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,\
    .commandBufferCount = 1

#define DEFAULT_VK_ATTACHMENT\
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,\
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,\
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED

#define DEFAULT_VK_RENDER_PASS\
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,\
    .subpassCount = 1

#define DEFAULT_VK_SHADER_STAGE\
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,\
    .pName = "main"

#define DEFAULT_VK_RASTERIZATION\
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,\
    .depthClampEnable = VK_FALSE,\
    .rasterizerDiscardEnable = VK_FALSE,\
    .polygonMode = VK_POLYGON_MODE_FILL,\
    .lineWidth = 1.0f,\
    .cullMode = VK_CULL_MODE_BACK_BIT,\
    .frontFace = VK_FRONT_FACE_CLOCKWISE,\
    .depthBiasEnable = VK_FALSE

#define DEFAULT_VK_MULTISAMPLE\
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,\
    .sampleShadingEnable = VK_FALSE,\
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT

#define DEFAULT_VK_GRAPHICS_PIPELINE\
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,\
    .pInputAssemblyState = &default_input_assembly_create_info,\
    .pViewportState = &default_viewport_create_info,\
    .pDepthStencilState = &default_depth_stencil_create_info,\
    .pColorBlendState = &default_color_blend_create_info,\
    .pDynamicState = &default_dynamic_create_info,\
    .subpass = 0,\
    .basePipelineHandle = VK_NULL_HANDLE,\
    .basePipelineIndex = -1

#define DEFAULT_VK_SAMPLER\
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,\
    .minFilter = VK_FILTER_LINEAR,\
    .magFilter = VK_FILTER_LINEAR,\
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,\
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,\
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,\
    .anisotropyEnable = VK_TRUE,\
    .maxAnisotropy = physical_device_properties->limits.maxSamplerAnisotropy,\
    .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,\
    .unnormalizedCoordinates = VK_FALSE,\
    .compareEnable = VK_FALSE,\
    .compareOp = VK_COMPARE_OP_ALWAYS,\
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,\
    .minLod = 0.0f,\
    .maxLod = 0.0f,\
    .mipLodBias = 0.0f



