#include "defaults.h"
#include "util.h"
#include <stdalign.h>

alignas(64)

const VkPipelineInputAssemblyStateCreateInfo default_input_assembly_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE
};

const VkPipelineViewportStateCreateInfo default_viewport_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .scissorCount = 1
};

const VkPipelineDepthStencilStateCreateInfo default_depth_stencil_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .depthTestEnable = VK_TRUE,
    .depthWriteEnable = VK_TRUE,
    .depthCompareOp = VK_COMPARE_OP_LESS,
    .depthBoundsTestEnable = VK_FALSE, // TODO: Use this?
    .stencilTestEnable = VK_FALSE
};

static VkPipelineColorBlendAttachmentState color_blend_attachments[] = {
    {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE
    }
};
const VkPipelineColorBlendStateCreateInfo default_color_blend_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable = VK_FALSE,
    .attachmentCount = NUM_ELEMS(color_blend_attachments),
    .pAttachments = color_blend_attachments
};

static VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
const VkPipelineDynamicStateCreateInfo default_dynamic_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = NUM_ELEMS(dynamic_states),
    .pDynamicStates = dynamic_states
};

const VkBufferCreateInfo vertex_buffer_create_info = {
    DEFAULT_VK_BUFFER,
    .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
};

const VkBufferCreateInfo index_buffer_create_info = {
    DEFAULT_VK_BUFFER,
    .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
};

const VkBufferCreateInfo uniform_buffer_create_info = {
    DEFAULT_VK_BUFFER,
    .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
};

const VmaAllocationCreateInfo staging_allocation_create_info = {
    DEFAULT_VMA_ALLOCATION,
    .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
};

const VmaAllocationCreateInfo device_allocation_create_info = {
    DEFAULT_VMA_ALLOCATION,
    .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
};