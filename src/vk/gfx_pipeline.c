#include "gfx_pipeline.h"
#include "core.h"
#include "ds.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static VkShaderModule create_shader_module(const char* path) {
    if (access(path, F_OK) != 0) {
        return VK_NULL_HANDLE;
    }

    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return VK_NULL_HANDLE;
    }

    struct stat st;
    stat(path, &st);

    size_t aligned_num_bytes = st.st_size % 32ul == 0 ? st.st_size : st.st_size + (32ul - (st.st_size % 32ul));

    uint32_t* bytes = ds_promise(aligned_num_bytes);
    memset(bytes, 0, aligned_num_bytes);
    if (fread(bytes, st.st_size, 1, file) != 1) {
        return VK_NULL_HANDLE;
    }

    fclose(file);

    VkShaderModuleCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = st.st_size,
        .pCode = bytes
    };

    VkShaderModule shader_module;
    return vkCreateShaderModule(device, &info, NULL, &shader_module) == VK_SUCCESS ? shader_module : VK_NULL_HANDLE;
}

static uint32_t get_memory_type_index(uint32_t memory_type_bits, VkMemoryPropertyFlags property_flags) {
    VkPhysicalDeviceMemoryProperties properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &properties);

    for (uint32_t i = 0; i < properties.memoryTypeCount; i++) {
        if ((memory_type_bits & (1u << i)) && (properties.memoryTypes[i].propertyFlags & property_flags) == property_flags) {
            return i;
        }
    }

    return NULL_UINT32;
}

// TODO: Use VulkanMemoryAllocator
static result_t create_buffer(VkDeviceSize num_buffer_bytes, VkBufferUsageFlags usage, VkMemoryPropertyFlags property_flags, VkBuffer* buffer, VkDeviceMemory* buffer_memory) {
    {
        VkBufferCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = num_buffer_bytes,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        if (vkCreateBuffer(device, &info, NULL, buffer) != VK_SUCCESS) {
            return result_success;
        }
    }

    {
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(device, *buffer, &requirements);

        VkMemoryAllocateInfo info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = requirements.size,
            .memoryTypeIndex = get_memory_type_index(requirements.memoryTypeBits, property_flags)
        };

        if (vkAllocateMemory(device, &info, NULL, buffer_memory) != VK_SUCCESS) {
            return result_failure;
        }
    }

    if (vkBindBufferMemory(device, *buffer, *buffer_memory, 0) != VK_SUCCESS) {
        return result_failure;
    }

    return result_success;
}

const char* init_vulkan_graphics_pipeline(void) {
    //
    
    VkAttachmentDescription color_attachment = {
        .format = surface_format.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference color_attachment_reference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_reference
    };

    VkSubpassDependency subpass_dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    };

    {
        VkRenderPassCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &color_attachment,
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 1,
            .pDependencies = &subpass_dependency
        };

        if (vkCreateRenderPass(device, &info, NULL, &render_pass) != VK_SUCCESS) {
            return "Failed to create render pass\n";
        }
    }

    //


    //
    if (create_buffer(sizeof(vertices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertex_buffer, &vertex_buffer_memory) != result_success) {
        return "Failed to create vertex buffer\n";
    }
    if (create_buffer(sizeof(vertex_indices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &index_buffer, &index_buffer_memory) != result_success) {
        return "Failed to create index buffer\n";
    }

    VkBuffer vertex_staging_buffer;
    VkDeviceMemory vertex_staging_buffer_memory;
    if (create_buffer(sizeof(vertices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertex_staging_buffer, &vertex_staging_buffer_memory) != result_success) {
        return "Failed to create vertex staging buffer\n";
    }

    void* mapped_vertices;
    if (vkMapMemory(device, vertex_staging_buffer_memory, 0, sizeof(vertices), 0, &mapped_vertices) != VK_SUCCESS) {
        return "Failed to map vertex staging buffer memory\n";
    }
    memcpy(mapped_vertices, vertices, sizeof(vertices));
    vkUnmapMemory(device, vertex_staging_buffer_memory);

    VkBuffer index_staging_buffer;
    VkDeviceMemory index_staging_buffer_memory;
    if (create_buffer(sizeof(vertex_indices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &index_staging_buffer, &index_staging_buffer_memory) != result_success) {
        return "Failed to create index staging buffer\n";
    }

    void* mapped_vertex_indices;
    if (vkMapMemory(device, index_staging_buffer_memory, 0, sizeof(vertex_indices), 0, &mapped_vertex_indices) != VK_SUCCESS) {
        return "Failed to map index staging buffer memory\n";
    }
    memcpy(mapped_vertex_indices, vertex_indices, sizeof(vertex_indices));
    vkUnmapMemory(device, index_staging_buffer_memory);

    VkCommandBuffer command_buffer;
    {
        VkCommandBufferAllocateInfo info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandPool = command_pool, // TODO: Use separate command pool
            .commandBufferCount = 1
        };

        if (vkAllocateCommandBuffers(device, &info, &command_buffer) != VK_SUCCESS) {
            return "Failed to create transfer command buffer\n";
        }
    }

    {
        VkCommandBufferBeginInfo info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };
        if (vkBeginCommandBuffer(command_buffer, &info) != VK_SUCCESS) {
            return "Failed to write to transfer command buffer\n";
        }
    }

    {
        VkBufferCopy region = {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = sizeof(vertices)
        };
        vkCmdCopyBuffer(command_buffer, vertex_staging_buffer, vertex_buffer, 1, &region);
    }
    {
        VkBufferCopy region = {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = sizeof(vertex_indices)
        };
        vkCmdCopyBuffer(command_buffer, index_staging_buffer, index_buffer, 1, &region);
    }

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        return "Failed to write to transfer command buffer\n";
    }

    {
        VkSubmitInfo info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffer
        };

        // TODO: Use a fence
        vkQueueSubmit(graphics_queue, 1, &info, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphics_queue);
    }

    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);

    vkDestroyBuffer(device, vertex_staging_buffer, NULL);
    vkFreeMemory(device, vertex_staging_buffer_memory, NULL);
    vkDestroyBuffer(device, index_staging_buffer, NULL);
    vkFreeMemory(device, index_staging_buffer_memory, NULL);

    //

    for (size_t i = 0; i < NUM_FRAMES_IN_FLIGHT; i++) {
        if (create_buffer(sizeof(clip_space_matrix), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniform_buffers[i], &uniform_buffers_memory[i]) != result_success) {
            return "Failed to create uniform buffer\n";
        }
        if (vkMapMemory(device, uniform_buffers_memory[i], 0, sizeof(clip_space_matrix), 0, &mapped_clip_space_matrices[i]) != VK_SUCCESS) {
            return "Failed to map uniform buffer memory\n";
        }
    }

    //

    //

    VkDescriptorSetLayoutBinding uniform_buffer_layout_binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = NULL
    };

    {
        VkDescriptorSetLayoutCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 1,
            .pBindings = &uniform_buffer_layout_binding
        };

        if (vkCreateDescriptorSetLayout(device, &info, NULL, &descriptor_set_layout) != VK_SUCCESS) {
            return "Failed to create descriptor set layout\n";
        }
    }

    {
        VkDescriptorPoolSize size = {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = NUM_FRAMES_IN_FLIGHT
        };

        VkDescriptorPoolCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .poolSizeCount = 1,
            .pPoolSizes = &size,
            .maxSets = NUM_FRAMES_IN_FLIGHT
        };

        if (vkCreateDescriptorPool(device, &info, NULL, &descriptor_pool) != VK_SUCCESS) {
            return "Failed to create descriptor pool\n";
        }
    }

    {
        VkDescriptorSetLayout layouts[NUM_FRAMES_IN_FLIGHT];
        for (size_t i = 0; i < NUM_FRAMES_IN_FLIGHT; i++) {
            layouts[i] = descriptor_set_layout;
        }

        VkDescriptorSetAllocateInfo info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = descriptor_pool,
            .descriptorSetCount = NUM_FRAMES_IN_FLIGHT,
            .pSetLayouts = layouts
        };

        if (vkAllocateDescriptorSets(device, &info, descriptor_sets) != VK_SUCCESS) {
            return "Failed to allocate descriptor sets\n";
        }
    }

    for (size_t i = 0; i < NUM_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo info = {
            .buffer = uniform_buffers[i],
            .offset = 0,
            .range = sizeof(clip_space_matrix)
        };

        VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_sets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .pBufferInfo = &info
        };

        vkUpdateDescriptorSets(device, 1, &write, 0, NULL);
    }

    //

    VkShaderModule vertex_shader_module = create_shader_module("shader/vertex.spv");
    if (vertex_shader_module == VK_NULL_HANDLE) {
        return "Failed to create vertex shader module\n";
    }

    VkShaderModule fragment_shader_module = create_shader_module("shader/fragment.spv");
    if (fragment_shader_module == VK_NULL_HANDLE) {
        return "Failed to create vertex shader module\n";
    }
    
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

    //

    VkVertexInputBindingDescription vertex_input_binding_description = {
        .binding = 0,
        .stride = sizeof(vertex_t),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    VkVertexInputAttributeDescription vertex_input_attribute_descriptions[] = {
        {
            .binding = 0,
            .location = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(vertex_t, position)
        },
        {
            .binding = 0,
            .location = 1,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(vertex_t, color)
        }
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_pipeline_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertex_input_binding_description,
        .vertexAttributeDescriptionCount = 2,
        .pVertexAttributeDescriptions = vertex_input_attribute_descriptions,
    };

    //

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

    {
        VkPipelineLayoutCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &descriptor_set_layout
        };

        if (vkCreatePipelineLayout(device, &info, NULL, &pipeline_layout) != VK_SUCCESS) {
            return "Failed to create pipeline layout\n";
        }
    }

    {
        VkGraphicsPipelineCreateInfo info = {
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

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, NULL, &pipeline) != VK_SUCCESS) {
            return "Failed to create graphics pipeline\n";
        }
    }

    vkDestroyShaderModule(device, vertex_shader_module, NULL);
    vkDestroyShaderModule(device, fragment_shader_module, NULL);

    return NULL;
}