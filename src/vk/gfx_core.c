#include "gfx_core.h"
#include "core.h"
#include "util.h"
#include "ds.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stb_image.h>
#include <math.h>

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

result_t write_to_buffer(VmaAllocation buffer_allocation, size_t num_bytes, const void* data) {
    void* mapped_data;
    if (vmaMapMemory(allocator, buffer_allocation, &mapped_data) != VK_SUCCESS) {
        return result_failure;
    }
    memcpy(mapped_data, data, num_bytes);
    vmaUnmapMemory(allocator, buffer_allocation);

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
    size_t num_shaders, const shader_t shaders[],
    size_t num_descriptor_bindings, const descriptor_binding_t descriptor_bindings[], const descriptor_info_t descriptor_infos[],
    size_t num_vertex_bindings, const uint32_t num_vertex_bytes_array[],
    size_t num_vertex_attributes, const vertex_attribute_t vertex_attributes[],
    size_t num_push_constants_bytes,
    VkSampleCountFlagBits multisample_flags, depth_bias_t depth_bias,
    VkRenderPass render_pass,
    VkDescriptorSetLayout* descriptor_set_layout, VkDescriptorPool* descriptor_pool, VkDescriptorSet* descriptor_set, VkPipelineLayout* pipeline_layout, VkPipeline* pipeline
) {
    // TODO: This is a hack, later just separate this out into it's own function
    if (num_descriptor_bindings > 0) {
        {
            VkDescriptorSetLayoutBinding* bindings = ds_promise(num_descriptor_bindings*sizeof(VkDescriptorSetLayoutBinding));
            for (size_t i = 0; i < num_descriptor_bindings; i++) {
                descriptor_binding_t binding = descriptor_bindings[i];
                bindings[i] = (VkDescriptorSetLayoutBinding) {
                    .binding = i,
                    .descriptorType = binding.type,
                    .descriptorCount = 1,
                    .stageFlags = binding.stage_flags,
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
    }

    //

    VkVertexInputBindingDescription* vertex_input_binding_descriptions = ds_push(num_vertex_bindings*sizeof(VkVertexInputBindingDescription));

    for (size_t i = 0; i < num_vertex_bindings; i++) {
        vertex_input_binding_descriptions[i] = (VkVertexInputBindingDescription) {
            .binding = i,
            .stride = num_vertex_bytes_array[i],
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        };
    }

    VkVertexInputAttributeDescription* vertex_input_attribute_descriptions = ds_push(num_vertex_attributes*sizeof(VkVertexInputAttributeDescription));
    for (size_t i = 0; i < num_vertex_attributes; i++) {
        vertex_attribute_t attribute = vertex_attributes[i];

        vertex_input_attribute_descriptions[i] = (VkVertexInputAttributeDescription) {
            .binding = attribute.binding,
            .location = i,
            .format = attribute.format,
            .offset = attribute.offset
        };
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_pipeline_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = num_vertex_bindings,
        .pVertexBindingDescriptions = vertex_input_binding_descriptions,
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
            .setLayoutCount = num_descriptor_bindings != 0 ? 1 : 0,  // TODO: Also a hack
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
        .depthBiasEnable = depth_bias.enable,
        .depthBiasConstantFactor = depth_bias.constant_factor,
        .depthBiasSlopeFactor = depth_bias.slope_factor
    };

    VkPipelineMultisampleStateCreateInfo multisample_pipeline_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = multisample_flags
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

    VkPipelineShaderStageCreateInfo* shader_pipeline_stage_create_infos = ds_promise(num_shaders*sizeof(VkPipelineShaderStageCreateInfo));
    for (size_t i = 0; i < num_shaders; i++) {
        shader_t shader = shaders[i];

        shader_pipeline_stage_create_infos[i] = (VkPipelineShaderStageCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = shader.stage_flags,
            .module = shader.module,
            .pName = "main",
            .pSpecializationInfo = NULL
        };
    }

    {
        VkGraphicsPipelineCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = num_shaders,
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

    ds_restore(vertex_input_binding_descriptions);

    return NULL;
}

result_t begin_images(size_t num_images, const char* image_paths[], const VkFormat formats[], image_extent_t image_extents[], uint32_t num_mip_levels_array[], VkBuffer image_staging_buffers[], VmaAllocation image_staging_allocations[], VkImage images[], VmaAllocation image_allocations[]) {
    int image_channels;

    for (size_t i = 0; i < num_images; i++) {
        image_extent_t image_extent;
        stbi_uc* pixels = stbi_load(image_paths[i], &image_extent.width, &image_extent.height, &image_channels, STBI_rgb_alpha);
        if (pixels == NULL) {
            return result_failure;
        }
        size_t num_image_bytes = image_extent.width*image_extent.height*4;

        image_extents[i] = image_extent;
        uint32_t num_mip_levels = ((uint32_t)floorf(log2f(max_uint32(image_extent.width, image_extent.height)))) + 1;
        num_mip_levels_array[i] = num_mip_levels;

        if (create_buffer(num_image_bytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &image_staging_buffers[i], &image_staging_allocations[i]) != result_success) {
            return result_failure;
        }

        if (write_to_buffer(image_staging_allocations[i], num_image_bytes, pixels) != result_success) {
            return result_failure;
        }

        stbi_image_free(pixels);

        if (create_image(image_extent.width, image_extent.height, num_mip_levels, formats[i], VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT |VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &images[i], &image_allocations[i]) != result_success) {
            return result_failure;
        }
    }
    
    return result_success;
}

void transfer_images(VkCommandBuffer command_buffer, size_t num_images, const image_extent_t image_extents[], const uint32_t num_mip_levels_array[], const VkBuffer image_staging_buffers[], const VkImage images[]) {
    for (size_t i = 0; i < num_images; i++) {
        image_extent_t image_extent = image_extents[i];
        uint32_t num_mip_levels = num_mip_levels_array[i];
        VkBuffer image_staging_buffer = image_staging_buffers[i];
        VkImage image = images[i];

        transition_image_layout(command_buffer, image, num_mip_levels, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        transfer_from_staging_buffer_to_image(command_buffer, image_extent.width, image_extent.height, image_staging_buffer, image);

        uint32_t mip_width = image_extent.width;
        uint32_t mip_height = image_extent.height;

        for (size_t i = 1; i < num_mip_levels; i++) {
            transition_image_layout(command_buffer, image, 1, i - 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

            VkImageBlit blit = {
                .srcOffsets[0] = { 0, 0, 0 },
                .srcOffsets[1] = { mip_width, mip_height, 1 },
                .srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .srcSubresource.mipLevel = i - 1,
                .srcSubresource.baseArrayLayer = 0,
                .srcSubresource.layerCount = 1,
                .dstOffsets[0] = { 0, 0, 0 },
                .dstOffsets[1] = { mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1 },
                .dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .dstSubresource.mipLevel = i,
                .dstSubresource.baseArrayLayer = 0,
                .dstSubresource.layerCount = 1
            };

            vkCmdBlitImage(command_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

            transition_image_layout(command_buffer, image, 1, i - 1, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

            if (mip_width > 1) { mip_width /= 2; }
            if (mip_height > 1) { mip_height /= 2; }
        }

        transition_image_layout(command_buffer, image, 1, num_mip_levels - 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }
}

void end_images(size_t num_images, const VkBuffer image_staging_buffers[], const VmaAllocation image_staging_buffer_allocations[]) {
    for (size_t i = 0; i < num_images; i++) {
        vmaDestroyBuffer(allocator, image_staging_buffers[i], image_staging_buffer_allocations[i]);
    }
}

result_t create_image_views(size_t num_images, const VkFormat formats[], const uint32_t num_mip_levels_array[], const VkImage images[], VkImageView image_views[]) {
    for (size_t i = 0; i < num_images; i++) {
        if (create_image_view(images[i], num_mip_levels_array[i], formats[i], VK_IMAGE_ASPECT_COLOR_BIT, &image_views[i]) != result_success) {
            return result_failure;
        }
    }

    return result_success;
}

void destroy_images(size_t num_images, const VkImage images[], const VmaAllocation image_allocations[], const VkImageView image_views[]) {
    for (size_t i = 0; i < num_images; i++) {
        vkDestroyImageView(device, image_views[i], NULL);

        vmaDestroyImage(allocator, images[i], image_allocations[i]);
    }
}

void draw_scene(
    VkCommandBuffer command_buffer,
    VkFramebuffer image_framebuffer, VkExtent2D image_extent,
    size_t num_clear_values, const VkClearValue clear_values[],
    VkRenderPass render_pass, VkDescriptorSet descriptor_set, VkPipelineLayout pipeline_layout, VkPipeline pipeline,
    size_t num_push_constants_bytes, const void* push_constants,
    size_t num_vertex_buffers, const VkBuffer vertex_buffers[],
    size_t num_indices, VkBuffer index_buffer,
    size_t num_instances
) {
    {
        VkRenderPassBeginInfo info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = render_pass,
            .framebuffer = image_framebuffer,
            .renderArea.offset = { 0, 0 },
            .renderArea.extent = image_extent,
            .clearValueCount = num_clear_values,
            .pClearValues = clear_values
        };

        vkCmdBeginRenderPass(command_buffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkDeviceSize* offsets = ds_promise(num_vertex_buffers*sizeof(VkDeviceSize));
    memset(offsets, 0, num_vertex_buffers*sizeof(VkDeviceSize));
    vkCmdBindVertexBuffers(command_buffer, 0, num_vertex_buffers, vertex_buffers, offsets);

    vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT16);
    
    if (descriptor_set != NULL) {
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, NULL);
    }
    if (num_push_constants_bytes > 0) {
        vkCmdPushConstants(command_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, num_push_constants_bytes, push_constants);
    }

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = image_extent.width,
        .height = image_extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = { 0, 0 },
        .extent = image_extent
    };
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    vkCmdDrawIndexed(command_buffer, num_indices, num_instances, 0, 0, 0);

    vkCmdEndRenderPass(command_buffer);
}

result_t begin_vertex_arrays(
    size_t num_vertices,
    size_t num_vertex_arrays, void* const vertex_arrays[], const size_t num_vertex_bytes_array[], VkBuffer vertex_staging_buffers[], VmaAllocation vertex_staging_buffer_allocations[], VkBuffer vertex_buffers[], VmaAllocation vertex_buffer_allocations[]
) {
    for (size_t i = 0; i < num_vertex_arrays; i++) {
        void* vertex_array = vertex_arrays[i];
        size_t num_vertex_array_bytes = num_vertices*num_vertex_bytes_array[i];

        if (create_buffer(num_vertex_array_bytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertex_buffers[i], &vertex_buffer_allocations[i]) != result_success) {
            return result_failure;
        }

        if (create_buffer(num_vertex_array_bytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertex_staging_buffers[i], &vertex_staging_buffer_allocations[i]) != result_success) {
            return result_failure;
        }

        if (write_to_buffer(vertex_staging_buffer_allocations[i], num_vertex_array_bytes, vertex_array) != result_success) {
            return result_failure;
        }

        free(vertex_array);
    }

    return result_success;
}

void transfer_vertex_arrays(VkCommandBuffer command_buffer, size_t num_vertices, size_t num_vertex_arrays, const size_t num_vertex_bytes_array[], const VkBuffer vertex_staging_buffers[], const VkBuffer vertex_buffers[]) {
    for (size_t i = 0; i < num_vertex_arrays; i++) {
        transfer_from_staging_buffer_to_buffer(command_buffer, num_vertices*num_vertex_bytes_array[i], vertex_staging_buffers[i], vertex_buffers[i]);
    }
}

void end_vertex_arrays(size_t num_vertex_arrays, const VkBuffer vertex_staging_buffers[], const VmaAllocation vertex_staging_buffer_allocations[]) {
    for (size_t i = 0; i < num_vertex_arrays; i++) {
        vmaDestroyBuffer(allocator, vertex_staging_buffers[i], vertex_staging_buffer_allocations[i]);
    }
}

result_t begin_indices(size_t num_index_bytes, size_t num_indices, void* indices, VkBuffer* index_staging_buffer, VmaAllocation* index_staging_buffer_allocation, VkBuffer* index_buffer, VmaAllocation* index_buffer_allocation) {
    size_t num_index_array_bytes = num_indices*num_index_bytes;

    if (create_buffer(num_index_array_bytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_buffer, index_buffer_allocation) != result_success) {
        return result_failure;
    }

    if (create_buffer(num_index_array_bytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, index_staging_buffer, index_staging_buffer_allocation) != result_success) {
        return result_failure;
    }

    if (write_to_buffer(*index_staging_buffer_allocation, num_index_array_bytes, indices) != result_success) {
        return result_failure;
    }

    free(indices);

    return result_success;
}

void transfer_indices(VkCommandBuffer command_buffer, size_t num_index_bytes, size_t num_indices, VkBuffer index_staging_buffer, VkBuffer index_buffer) {
    transfer_from_staging_buffer_to_buffer(command_buffer, num_indices*num_index_bytes, index_staging_buffer, index_buffer);
}

void end_indices(VkBuffer index_staging_buffer, VmaAllocation index_staging_buffer_allocation) {
    vmaDestroyBuffer(allocator, index_staging_buffer, index_staging_buffer_allocation);
}