#include "gfx_core.h"
#include "core.h"
#include "util.h"
#include "ds.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

result_t create_shader_module(const char* path, VkShaderModule* shader_module) {
    if (access(path, F_OK) != 0) {
        return result_failure;
    }

    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return result_failure;
    }

    struct stat st;
    stat(path, &st);

    size_t aligned_num_bytes = st.st_size % 32ul == 0 ? st.st_size : st.st_size + (32ul - (st.st_size % 32ul));

    uint32_t* bytes = ds_promise(aligned_num_bytes);
    memset(bytes, 0, aligned_num_bytes);
    if (fread(bytes, st.st_size, 1, file) != 1) {
        return result_failure;
    }

    fclose(file);

    VkShaderModuleCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = st.st_size,
        .pCode = bytes
    };

    if (vkCreateShaderModule(device, &info, NULL, shader_module) != VK_SUCCESS) {
        return result_failure;
    }
    return result_success;
}

result_t create_buffer(VkDeviceSize num_buffer_bytes, VkBufferUsageFlags usage_flags, VmaAllocationCreateFlags allocation_flags, VkMemoryPropertyFlags property_flags, VkBuffer* buffer, VmaAllocation* buffer_allocation) {
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = num_buffer_bytes,
        .usage = usage_flags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VmaAllocationCreateInfo allocation_info = {
        .usage = VMA_MEMORY_USAGE_AUTO,
        .flags = allocation_flags,
        .requiredFlags = property_flags
    };

    if (vmaCreateBuffer(allocator, &buffer_info, &allocation_info, buffer, buffer_allocation, NULL) != VK_SUCCESS) {
        return result_failure;
    }

    return result_success;
}

result_t create_mapped_buffer(VkDeviceSize num_buffer_bytes, VkBufferUsageFlags usage_flags, VmaAllocationCreateFlags allocation_flags, VkMemoryPropertyFlags property_flags, VkBuffer* buffer, VmaAllocation* buffer_allocation, void** mapped_data) {
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = num_buffer_bytes,
        .usage = usage_flags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VmaAllocationCreateInfo allocation_create_info = {
        .usage = VMA_MEMORY_USAGE_AUTO,
        .flags = allocation_flags | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .requiredFlags = property_flags
    };
    
    VmaAllocationInfo allocation_info = {};

    if (vmaCreateBuffer(allocator, &buffer_info, &allocation_create_info, buffer, buffer_allocation, &allocation_info) != VK_SUCCESS) {
        return result_failure;
    }

    *mapped_data = allocation_info.pMappedData;

    return result_success;
}

result_t write_to_staging_buffer(VmaAllocation staging_buffer_allocation, size_t num_bytes, const void* data) {
    void* mapped_data;
    if (vmaMapMemory(allocator, staging_buffer_allocation, &mapped_data) != VK_SUCCESS) {
        return result_failure;
    }
    memcpy(mapped_data, data, num_bytes);
    vmaUnmapMemory(allocator, staging_buffer_allocation);

    return result_success;
}

result_t create_image(uint32_t image_width, uint32_t image_height, uint32_t num_mip_levels, VkFormat format, VkSampleCountFlagBits multisample_flags, VkImageTiling tiling, VkImageUsageFlags usage_flags, VkMemoryPropertyFlags property_flags, VkImage* image, VmaAllocation* image_allocation) {
    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .extent.width = image_width,
        .extent.height = image_height,
        .extent.depth = 1,
        .mipLevels = num_mip_levels,
        .arrayLayers = 1,
        .format = format,
        .tiling = tiling,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage = usage_flags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .samples = multisample_flags
    };

    VmaAllocationCreateInfo allocation_info = {
        .usage = VMA_MEMORY_USAGE_AUTO,
        .flags = /*allocation_flags*/0,
        .requiredFlags = property_flags
    };

    if (vmaCreateImage(allocator, &image_info, &allocation_info, image, image_allocation, NULL) != VK_SUCCESS) {

    }

    return result_success;
}

void transfer_from_staging_buffer_to_buffer(VkCommandBuffer command_buffer, size_t num_bytes, VkBuffer staging_buffer, VkBuffer buffer) {
    VkBufferCopy region = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = num_bytes
    };
    vkCmdCopyBuffer(command_buffer, staging_buffer, buffer, 1, &region);
}

void transfer_from_staging_buffer_to_image(VkCommandBuffer command_buffer, uint32_t image_width, uint32_t image_height, VkBuffer staging_buffer, VkImage image) {
    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .imageSubresource.mipLevel = 0,
        .imageSubresource.baseArrayLayer = 0,
        .imageSubresource.layerCount = 1,
        .imageOffset = { 0, 0, 0 },
        .imageExtent.width = image_width,
        .imageExtent.height = image_height,
        .imageExtent.depth = 1
    };

    vkCmdCopyBufferToImage(command_buffer, staging_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void transition_image_layout(VkCommandBuffer command_buffer, VkImage image, uint32_t num_mip_levels, uint32_t mip_level_index, VkImageLayout old_layout, VkImageLayout new_layout, VkAccessFlags src_access_flags, VkAccessFlags dest_access_flags, VkPipelineStageFlags src_stage_flags, VkPipelineStageFlags dest_stage_flags) {
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel = mip_level_index,
        .subresourceRange.levelCount = num_mip_levels,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
        .srcAccessMask = src_access_flags,
        .dstAccessMask = dest_access_flags
    };

    vkCmdPipelineBarrier(command_buffer, src_stage_flags, dest_stage_flags, 0, 0, NULL, 0, NULL, 1, &barrier);
}

result_t create_image_view(VkImage image, uint32_t num_mip_levels, VkFormat format, VkImageAspectFlags aspect_flags, VkImageView* image_view) {
    VkImageViewCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
        .subresourceRange.aspectMask = aspect_flags,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = num_mip_levels,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1
    };

    if (vkCreateImageView(device, &info, NULL, image_view) != VK_SUCCESS) {
        return result_failure;
    }

    return result_success;
}

const char* create_graphics_pipeline(
    VkShaderModule vertex_shader_module, VkShaderModule fragment_shader_module,
    size_t num_descriptor_bindings, const descriptor_binding_t descriptor_bindings[], const descriptor_info_t descriptor_infos[],
    size_t num_vertex_bytes, size_t num_vertex_attributes, const vertex_attribute_t vertex_attributes[], 
    size_t num_push_constants_bytes,
    VkRenderPass render_pass,
    VkDescriptorSetLayout* descriptor_set_layout, VkDescriptorPool* descriptor_pool, VkDescriptorSet* descriptor_set, VkPipelineLayout* pipeline_layout, VkPipeline* pipeline
) {
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

    {
        VkDescriptorSetLayoutBinding* bindings = ds_promise(num_descriptor_bindings*sizeof(VkDescriptorSetLayoutBinding));
        for (size_t i = 0; i < num_descriptor_bindings; i++) {
            bindings[i] = (VkDescriptorSetLayoutBinding) {
                .binding = i,
                .descriptorType = descriptor_bindings[i].type,
                .descriptorCount = 1,
                .stageFlags = descriptor_bindings[i].stage_flags,
                .pImmutableSamplers = NULL
            };
        }

        VkDescriptorSetLayoutCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = num_descriptor_bindings,
            .pBindings = bindings
        };

        if (vkCreateDescriptorSetLayout(device, &info, NULL, descriptor_set_layout) != VK_SUCCESS) {
            return "Failed to create descriptor set layout\n";
        }
    }

    {
        VkDescriptorPoolSize* sizes = ds_promise(num_descriptor_bindings*sizeof(VkDescriptorPoolSize));
        for (size_t i = 0; i < num_descriptor_bindings; i++) {
            sizes[i] = (VkDescriptorPoolSize) {
                .type = descriptor_bindings[i].type,
                // .descriptorCount = NUM_FRAMES_IN_FLIGHT
                .descriptorCount = 1
            };
        }

        VkDescriptorPoolCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .poolSizeCount = num_descriptor_bindings,
            .pPoolSizes = sizes,
            .maxSets = 1
        };

        if (vkCreateDescriptorPool(device, &info, NULL, descriptor_pool) != VK_SUCCESS) {
            return "Failed to create descriptor pool\n";
        }
    }

    {
        VkDescriptorSetAllocateInfo info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = *descriptor_pool,
            .descriptorSetCount = 1,
            .pSetLayouts = descriptor_set_layout
        };

        if (vkAllocateDescriptorSets(device, &info, descriptor_set) != VK_SUCCESS) {
            return "Failed to allocate descriptor set\n";
        }
    }

    {
        VkWriteDescriptorSet* writes = ds_promise(num_descriptor_bindings*sizeof(VkWriteDescriptorSet));
        for (size_t i = 0; i < num_descriptor_bindings; i++) {
            VkWriteDescriptorSet write = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = *descriptor_set,
                .dstBinding = i,
                .dstArrayElement = 0,
                .descriptorType = descriptor_bindings[i].type,
                .descriptorCount = 1
            };
            const descriptor_info_t* info = &descriptor_infos[i];

            // TODO: This is bad
            if (info->type == descriptor_info_type_buffer) {
                write.pBufferInfo = &info->buffer;
            } else if (info->type == descriptor_info_type_image) {
                write.pImageInfo = &info->image;
            }

            writes[i] = write;
        }

        vkUpdateDescriptorSets(device, num_descriptor_bindings, writes, 0, NULL);
    }

    //

    VkVertexInputBindingDescription vertex_input_binding_description = {
        .binding = 0,
        .stride = num_vertex_bytes,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    VkVertexInputAttributeDescription* vertex_input_attribute_descriptions = ds_promise(num_vertex_attributes*sizeof(VkVertexInputAttributeDescription));
    for (size_t i = 0; i < num_vertex_attributes; i++) {
        vertex_input_attribute_descriptions[i] = (VkVertexInputAttributeDescription) {
            .binding = 0,
            .location = i,
            .format = vertex_attributes[i].format,
            .offset = vertex_attributes[i].offset
        };
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_pipeline_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertex_input_binding_description,
        .vertexAttributeDescriptionCount = num_vertex_attributes,
        .pVertexAttributeDescriptions = vertex_input_attribute_descriptions,
    };

    //

    {
        VkPushConstantRange range = {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = num_push_constants_bytes
        };

        VkPipelineLayoutCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = descriptor_set_layout,
            .pushConstantRangeCount = num_push_constants_bytes == 0 ? 0 : 1,
            .pPushConstantRanges = &range
        };

        if (vkCreatePipelineLayout(device, &info, NULL, pipeline_layout) != VK_SUCCESS) {
            return "Failed to create pipeline layout\n";
        }
    }

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
        .rasterizationSamples = render_multisample_flags
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil_pipeline_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE, // TODO: Use this?
        .stencilTestEnable = VK_FALSE
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
        VkGraphicsPipelineCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = NUM_ELEMS(shader_pipeline_stage_create_infos),
            .pStages = shader_pipeline_stage_create_infos,
            .pVertexInputState = &vertex_input_pipeline_state_create_info,
            .pInputAssemblyState = &input_assembly_pipeline_state_create_info,
            .pViewportState = &viewport_pipeline_state_create_info,
            .pRasterizationState = &rasterization_pipeline_state_create_info,
            .pMultisampleState = &multisample_pipeline_state_create_info,
            .pDepthStencilState = &depth_stencil_pipeline_state_create_info,
            .pColorBlendState = &color_blend_pipeline_state_create_info,
            .pDynamicState = &dynamic_pipeline_state_create_info,
            .layout = *pipeline_layout,
            .renderPass = render_pass,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1
        };

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, NULL, pipeline) != VK_SUCCESS) {
            return "Failed to create graphics pipeline\n";
        }
    }

    return NULL;
}