#include "gfx_pipeline.h"
#include "render_pass.h"
#include "core.h"
#include "ds.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>



typedef struct {
    union {
        uint32_t error;
        uint32_t num_bytes;
    };
    union {
        void* restore_point;
        uint32_t* bytes;
    };
} shader_bytecode_t;

static shader_bytecode_t read_shader_bytecode(const char* path) {
    if (access(path, F_OK) != 0) {
        return (shader_bytecode_t) { { .error = NULL_UINT32 } };
    }

    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return (shader_bytecode_t) { { .error = NULL_UINT32 } };
    }

    struct stat st;
    stat(path, &st);

    uint32_t aligned_num_bytes = st.st_size % 32 == 0 ? st.st_size : st.st_size + (32 - (st.st_size % 32));

    uint32_t* bytes = ds_push(aligned_num_bytes);
    memset(bytes, 0, aligned_num_bytes);
    if (fread(bytes, st.st_size, 1, file) != 1) {
        return (shader_bytecode_t) { { .error = NULL_UINT32 } };
    }

    fclose(file);

    return (shader_bytecode_t) { { .num_bytes = st.st_size }, { .bytes = bytes } };
}

static VkShaderModule init_shader_module(const shader_bytecode_t bytecode) {
    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = bytecode.num_bytes,
        .pCode = bytecode.bytes
    };

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device, &create_info, NULL, &shader_module) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return shader_module;
}

const char* init_vulkan_graphics_pipeline() {
    const shader_bytecode_t vertex_shader_bytecode = read_shader_bytecode("shader/vertex.spv");
    if (vertex_shader_bytecode.error == NULL_UINT32) {
        return "Failed to load vertex shader\n";
    }

    const shader_bytecode_t fragment_shader_bytecode = read_shader_bytecode("shader/fragment.spv");
    if (vertex_shader_bytecode.error == NULL_UINT32) {
        return "Failed to load fragment shader\n";
    }

    VkShaderModule vertex_shader_module = init_shader_module(vertex_shader_bytecode);
    if (vertex_shader_module == VK_NULL_HANDLE) {
        return "Failed to create vertex shader module\n";
    }

    VkShaderModule fragment_shader_module = init_shader_module(fragment_shader_bytecode);
    if (fragment_shader_module == VK_NULL_HANDLE) {
        return "Failed to create vertex shader module\n";
    }
    
    ds_restore(vertex_shader_bytecode.restore_point);

    VkPipelineShaderStageCreateInfo shader_pipeline_stage_create_infos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertex_shader_module,
            .pName = "main",
            .pSpecializationInfo = NULL
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragment_shader_module,
            .pName = "main",
            .pSpecializationInfo = NULL
        }
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_pipeline_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = NULL,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = NULL,
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_pipeline_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkPipelineViewportStateCreateInfo viewport_pipeline_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    VkPipelineRasterizationStateCreateInfo rasterization_pipeline_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL, // solid geometry, not wireframe
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE
    };

    VkPipelineMultisampleStateCreateInfo multisample_pipeline_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment_pipeline_state_create_info = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE
    };

    VkPipelineColorBlendStateCreateInfo color_blend_pipeline_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment_pipeline_state_create_info
    };

    VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamic_pipeline_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = NUM_ELEMS(dynamic_states),
        .pDynamicStates = dynamic_states
    };

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pushConstantRangeCount = 0
    };

    if (vkCreatePipelineLayout(device, &pipeline_layout_create_info, NULL, &pipeline_layout) != VK_SUCCESS) {
        return "Failed to create pipeline layout\n";
    }

    VkGraphicsPipelineCreateInfo pipeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shader_pipeline_stage_create_infos,
        .pVertexInputState = &vertex_input_pipeline_state_create_info,
        .pInputAssemblyState = &input_assembly_pipeline_state_create_info,
        .pViewportState = &viewport_pipeline_state_create_info,
        .pRasterizationState = &rasterization_pipeline_state_create_info,
        .pMultisampleState = &multisample_pipeline_state_create_info,
        .pDepthStencilState = NULL,
        .pColorBlendState = &color_blend_pipeline_state_create_info,
        .pDynamicState = &dynamic_pipeline_state_create_info,
        .layout = pipeline_layout,
        .renderPass = render_pass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &pipeline) != VK_SUCCESS) {
        return "Failed to create graphics pipeline\n";
    }

    vkDestroyShaderModule(device, vertex_shader_module, NULL);
    vkDestroyShaderModule(device, fragment_shader_module, NULL);

    return NULL;
}