#include "defaults.h"
#include "util.h"

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