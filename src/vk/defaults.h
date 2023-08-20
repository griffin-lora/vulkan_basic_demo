#pragma once
#include "vk.h"
#include <vk_mem_alloc.h>

extern const VkPipelineInputAssemblyStateCreateInfo default_input_assembly_create_info;
extern const VkPipelineViewportStateCreateInfo default_viewport_create_info;
extern const VkPipelineDepthStencilStateCreateInfo default_depth_stencil_create_info;
extern const VkPipelineColorBlendStateCreateInfo default_color_blend_create_info;
extern const VkPipelineDynamicStateCreateInfo default_dynamic_create_info;

extern const VkBufferCreateInfo default_vertex_buffer_create_info;
extern const VkBufferCreateInfo default_index_buffer_create_info;
extern const VkBufferCreateInfo default_uniform_buffer_create_info;
extern const VmaAllocationCreateInfo default_staging_allocation_create_info;
extern const VmaAllocationCreateInfo default_device_allocation_create_info;

#define DEFAULT_VK_COMMAND_BUFFER\
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,\
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,\
    .commandBufferCount = 1

#define DEFAULT_VK_ATTACHMENT\
    .samples = VK_SAMPLE_COUNT_1_BIT,\
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,\
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,\
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED

#define DEFAULT_VK_RENDER_PASS\
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,\
    .subpassCount = 1

#define DEFAULT_VK_DESCRIPTOR_BINDING\
    .descriptorCount = 1

#define DEFAULT_VK_PIPELINE_LAYOUT\
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,\
    .setLayoutCount = 1

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

#define DEFAULT_VK_BUFFER\
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,\
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE

#define DEFAULT_VK_STAGING_BUFFER\
    DEFAULT_VK_BUFFER,\
    .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT

#define DEFAULT_VMA_ALLOCATION\
    .usage = VMA_MEMORY_USAGE_AUTO

#define DEFAULT_VK_IMAGE\
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,\
    .imageType = VK_IMAGE_TYPE_2D,\
    .extent.depth = 1,\
    .mipLevels = 1,\
    .arrayLayers = 1,\
    .tiling = VK_IMAGE_TILING_OPTIMAL,\
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,\
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,\
    .samples = VK_SAMPLE_COUNT_1_BIT

#define DEFAULT_VK_BUFFER_IMAGE_COPY\
    .bufferOffset = 0,\
    .bufferRowLength = 0,\
    .bufferImageHeight = 0,\
    .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,\
    .imageSubresource.mipLevel = 0,\
    .imageSubresource.baseArrayLayer = 0,\
    .imageSubresource.layerCount = 1,\
    .imageOffset = { 0, 0, 0 },\
    .imageExtent.depth = 1

#define DEFAULT_VK_IMAGE_MEMORY_BARRIER\
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,\
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,\
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,\
    .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,\
    .subresourceRange.baseMipLevel = 0,\
    .subresourceRange.levelCount = 1,\
    .subresourceRange.baseArrayLayer = 0,\
    .subresourceRange.layerCount = 1,\
    .srcAccessMask = src_access_flags,\
    .dstAccessMask = dest_access_flags

#define DEFAULT_VK_IMAGE_BLIT\
    .srcOffsets[0] = { 0, 0, 0 },\
    .srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,\
    .srcSubresource.mipLevel = 0,\
    .srcSubresource.baseArrayLayer = 0,\
    .srcSubresource.layerCount = 1,\
    .dstOffsets[0] = { 0, 0, 0 },\
    .dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,\
    .dstSubresource.mipLevel = 0,\
    .dstSubresource.baseArrayLayer = 0,\
    .dstSubresource.layerCount = 1

#define DEFAULT_VK_IMAGE_VIEW\
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,\
    .viewType = VK_IMAGE_VIEW_TYPE_2D,\
    .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,\
    .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,\
    .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,\
    .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,\
    .subresourceRange.baseMipLevel = 0,\
    .subresourceRange.levelCount = 1,\
    .subresourceRange.baseArrayLayer = 0,\
    .subresourceRange.layerCount = 1

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

