#include "gfx_core.h"
#include "core.h"
#include "util.h"
#include "ds.h"
#include "defaults.h"
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

result_t write_to_buffer(VmaAllocation buffer_allocation, size_t num_bytes, const void* data) {
    void* mapped_data;
    if (vmaMapMemory(allocator, buffer_allocation, &mapped_data) != VK_SUCCESS) {
        return result_failure;
    }
    memcpy(mapped_data, data, num_bytes);
    vmaUnmapMemory(allocator, buffer_allocation);

    return result_success;
}

result_t writes_to_buffer(VmaAllocation buffer_allocation, size_t num_write_bytes, size_t num_writes, const void* const data_array[]) {
    void* mapped_data;
    if (vmaMapMemory(allocator, buffer_allocation, &mapped_data) != VK_SUCCESS) {
        return result_failure;
    }
    for (size_t i = 0; i < num_writes; i++) {
        memcpy(mapped_data + (i*num_write_bytes), data_array[i], num_write_bytes);
    }
    vmaUnmapMemory(allocator, buffer_allocation);

    return result_success;
}

result_t create_image(uint32_t image_width, uint32_t image_height, uint32_t num_mip_levels, uint32_t num_layers, VkFormat format, VkSampleCountFlagBits multisample_flags, VkImageTiling tiling, VkImageUsageFlags usage_flags, VkMemoryPropertyFlags property_flags, VkImage* image, VmaAllocation* image_allocation) {
    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .extent.width = image_width,
        .extent.height = image_height,
        .extent.depth = 1,
        .mipLevels = num_mip_levels,
        .arrayLayers = num_layers,
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
        return result_failure;
    }

    return result_success;
}

void transfer_from_staging_buffer_to_image(VkCommandBuffer command_buffer, uint32_t image_width, uint32_t image_height, uint32_t num_layers, VkBuffer staging_buffer, VkImage image) {
    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .imageSubresource.mipLevel = 0,
        .imageSubresource.baseArrayLayer = 0,
        .imageSubresource.layerCount = num_layers,
        .imageOffset = { 0, 0, 0 },
        .imageExtent.width = image_width,
        .imageExtent.height = image_height,
        .imageExtent.depth = 1
    };

    vkCmdCopyBufferToImage(command_buffer, staging_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void transition_image_layout(VkCommandBuffer command_buffer, VkImage image, uint32_t num_mip_levels, uint32_t mip_level_index, uint32_t num_layers, VkImageLayout old_layout, VkImageLayout new_layout, VkAccessFlags src_access_flags, VkAccessFlags dest_access_flags, VkPipelineStageFlags src_stage_flags, VkPipelineStageFlags dest_stage_flags) {
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
        .subresourceRange.layerCount = num_layers,
        .srcAccessMask = src_access_flags,
        .dstAccessMask = dest_access_flags
    };

    vkCmdPipelineBarrier(command_buffer, src_stage_flags, dest_stage_flags, 0, 0, NULL, 0, NULL, 1, &barrier);
}

result_t create_image_view(VkImage image, uint32_t num_mip_levels, uint32_t num_layers, VkFormat format, VkImageAspectFlags aspect_flags, VkImageView* image_view) {
    VkImageViewCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = num_layers == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY,
        .format = format,
        .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
        .subresourceRange.aspectMask = aspect_flags,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = num_mip_levels,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = num_layers
    };

    if (vkCreateImageView(device, &info, NULL, image_view) != VK_SUCCESS) {
        return result_failure;
    }

    return result_success;
}

result_t create_descriptor_set(VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info, descriptor_info_t descriptor_infos[], VkDescriptorSetLayout* descriptor_set_layout, VkDescriptorPool* descriptor_pool, VkDescriptorSet* descriptor_set) {
    if (vkCreateDescriptorSetLayout(device, &descriptor_set_layout_create_info, NULL, descriptor_set_layout) != VK_SUCCESS) {
        return result_failure;
    }

    size_t num_bindings = descriptor_set_layout_create_info.bindingCount;
    const VkDescriptorSetLayoutBinding* bindings = descriptor_set_layout_create_info.pBindings;

    {
        VkDescriptorPoolSize* sizes = ds_promise(num_bindings*sizeof(VkDescriptorPoolSize));
        for (size_t i = 0; i < num_bindings; i++) {
            sizes[i] = (VkDescriptorPoolSize) {
                .type = bindings[i].descriptorType,
                .descriptorCount = 1
            };
        }

        VkDescriptorPoolCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .poolSizeCount = num_bindings,
            .pPoolSizes = sizes,
            .maxSets = 1
        };

        if (vkCreateDescriptorPool(device, &info, NULL, descriptor_pool) != VK_SUCCESS) {
            return result_failure;
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
            return result_failure;
        }
    }

    {
        VkWriteDescriptorSet* writes = ds_promise(num_bindings*sizeof(VkWriteDescriptorSet));
        for (size_t i = 0; i < num_bindings; i++) {
            VkWriteDescriptorSet write = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = *descriptor_set,
                .dstBinding = bindings[i].binding,
                .dstArrayElement = 0,
                .descriptorType = bindings[i].descriptorType,
                .descriptorCount = 1
            };
            const descriptor_info_t* info = &descriptor_infos[i];

            if (info->type == descriptor_info_type_buffer) {
                write.pBufferInfo = &info->buffer;
            } else if (info->type == descriptor_info_type_image) {
                write.pImageInfo = &info->image;
            }

            writes[i] = write;
        }

        vkUpdateDescriptorSets(device, num_bindings, writes, 0, NULL);
    }

    return result_success;
}

result_t begin_images(size_t num_images, uint32_t num_layers, const char* image_paths[][num_layers], const VkFormat formats[], image_extent_t image_extents[], uint32_t num_mip_levels_array[], VkBuffer image_staging_buffers[], VmaAllocation image_staging_allocations[], VkImage images[], VmaAllocation image_allocations[]) {
    int image_channels;

    for (size_t i = 0; i < num_images; i++) {
        image_extent_t image_extent;

        const char** layer_paths = image_paths[i];

        stbi_uc** pixel_arrays = ds_promise(num_layers*sizeof(stbi_uc*));
        for (size_t j = 0; j < num_layers; j++) {
            image_extent_t new_image_extent;
            pixel_arrays[j] = stbi_load(layer_paths[j], &new_image_extent.width, &new_image_extent.height, &image_channels, STBI_rgb_alpha);
            if (pixel_arrays[j] == NULL) {
                return result_failure;
            }
            if (j > 0 && (new_image_extent.width != image_extent.width || new_image_extent.height != image_extent.height)) {
                return result_failure;
            }
            image_extent = new_image_extent;
        }
        
        VkDeviceSize num_layer_bytes = image_extent.width*image_extent.height*4;
        VkDeviceSize num_image_bytes = num_layers*num_layer_bytes;

        image_extents[i] = image_extent;
        uint32_t num_mip_levels = ((uint32_t)floorf(log2f(max_uint32(image_extent.width, image_extent.height)))) + 1;
        num_mip_levels_array[i] = num_mip_levels;

        {
            VkBufferCreateInfo info = {
                DEFAULT_VK_STAGING_BUFFER,
                .size = num_image_bytes
            };

            if (vmaCreateBuffer(allocator, &info, &default_staging_allocation_create_info, &image_staging_buffers[i], &image_staging_allocations[i], NULL) != VK_SUCCESS) {
                return result_failure;
            }
        }

        const void* const* pixel_arrays_cast = (const void* const*)pixel_arrays;
        if (writes_to_buffer(image_staging_allocations[i], num_layer_bytes, num_layers, pixel_arrays_cast) != result_success) {
            return result_failure;
        }

        for (size_t i = 0; i < num_layers; i++) {
            stbi_image_free(pixel_arrays[i]);
        }

        if (create_image(image_extent.width, image_extent.height, num_mip_levels, num_layers, formats[i], VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT |VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &images[i], &image_allocations[i]) != result_success) {
            return result_failure;
        }
    }
    
    return result_success;
}

void transfer_images(VkCommandBuffer command_buffer, size_t num_images, uint32_t num_layers, const image_extent_t image_extents[], const uint32_t num_mip_levels_array[], const VkBuffer image_staging_buffers[], const VkImage images[]) {
    for (size_t i = 0; i < num_images; i++) {
        image_extent_t image_extent = image_extents[i];
        uint32_t num_mip_levels = num_mip_levels_array[i];
        VkBuffer image_staging_buffer = image_staging_buffers[i];
        VkImage image = images[i];

        transition_image_layout(command_buffer, image, num_mip_levels, 0, num_layers, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        transfer_from_staging_buffer_to_image(command_buffer, image_extent.width, image_extent.height, num_layers, image_staging_buffer, image);

        uint32_t mip_width = image_extent.width;
        uint32_t mip_height = image_extent.height;

        for (size_t i = 1; i < num_mip_levels; i++) {
            transition_image_layout(command_buffer, image, 1, i - 1, num_layers, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

            VkImageBlit blit = {
                .srcOffsets[0] = { 0, 0, 0 },
                .srcOffsets[1] = { mip_width, mip_height, 1 },
                .srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .srcSubresource.mipLevel = i - 1,
                .srcSubresource.baseArrayLayer = 0,
                .srcSubresource.layerCount = num_layers,
                .dstOffsets[0] = { 0, 0, 0 },
                .dstOffsets[1] = { mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1 },
                .dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .dstSubresource.mipLevel = i,
                .dstSubresource.baseArrayLayer = 0,
                .dstSubresource.layerCount = num_layers
            };

            vkCmdBlitImage(command_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

            transition_image_layout(command_buffer, image, 1, i - 1, num_layers, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

            if (mip_width > 1) { mip_width /= 2; }
            if (mip_height > 1) { mip_height /= 2; }
        }

        transition_image_layout(command_buffer, image, 1, num_mip_levels - 1, num_layers, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }
}

void end_images(size_t num_images, const VkBuffer image_staging_buffers[], const VmaAllocation image_staging_buffer_allocations[]) {
    for (size_t i = 0; i < num_images; i++) {
        vmaDestroyBuffer(allocator, image_staging_buffers[i], image_staging_buffer_allocations[i]);
    }
}

result_t create_image_views(size_t num_images, uint32_t num_layers, const VkFormat formats[], const uint32_t num_mip_levels_array[], const VkImage images[], VkImageView image_views[]) {
    for (size_t i = 0; i < num_images; i++) {
        if (create_image_view(images[i], num_mip_levels_array[i], num_layers, formats[i], VK_IMAGE_ASPECT_COLOR_BIT, &image_views[i]) != result_success) {
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

void begin_pipeline(
    VkCommandBuffer command_buffer,
    VkFramebuffer image_framebuffer, VkExtent2D image_extent,
    uint32_t num_clear_values, const VkClearValue clear_values[],
    VkRenderPass render_pass, VkDescriptorSet descriptor_set, VkPipelineLayout pipeline_layout, VkPipeline pipeline
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
    
    if (descriptor_set != NULL) {
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, NULL);
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
}

void bind_vertex_buffers(VkCommandBuffer command_buffer, uint32_t num_vertex_buffers, const VkBuffer vertex_buffers[]) {
    VkDeviceSize* offsets = ds_promise(num_vertex_buffers*sizeof(VkDeviceSize));
    memset(offsets, 0, num_vertex_buffers*sizeof(VkDeviceSize));
    vkCmdBindVertexBuffers(command_buffer, 0, num_vertex_buffers, vertex_buffers, offsets);
}

void end_pipeline(VkCommandBuffer command_buffer) {
    vkCmdEndRenderPass(command_buffer);
}

result_t begin_buffers(
    VkDeviceSize num_elements, const VkBufferCreateInfo* base_device_buffer_create_info,
    size_t num_buffers, void* const arrays[], const VkDeviceSize num_element_bytes_array[], staging_t stagings[], VkBuffer buffers[], VmaAllocation allocations[]
) {
    for (size_t i = 0; i < num_buffers; i++) {
        void* array = arrays[i];
        VkDeviceSize num_array_bytes = num_elements*num_element_bytes_array[i];

        {
            VkBufferCreateInfo info = {
                DEFAULT_VK_STAGING_BUFFER,
                .size = num_array_bytes
            };

            if (vmaCreateBuffer(allocator, &info, &default_staging_allocation_create_info, &stagings[i].buffer, &stagings[i].allocation, NULL) != VK_SUCCESS) {
                return result_failure;
            }
        }
        
        {
            VkBufferCreateInfo info = *base_device_buffer_create_info;
            info.size = num_array_bytes;

            if (vmaCreateBuffer(allocator, &info, &default_device_allocation_create_info, &buffers[i], &allocations[i], NULL) != VK_SUCCESS) {
                return result_failure;
            }
        }

        if (write_to_buffer(stagings[i].allocation, num_array_bytes, array) != result_success) {
            return result_failure;
        }
    }
    
    return result_success;
}

void transfer_buffers(
    VkCommandBuffer command_buffer, VkDeviceSize num_elements,
    size_t num_buffers, const VkDeviceSize num_element_bytes_array[], const staging_t stagings[], const VkBuffer buffers[]
) {
    for (size_t i = 0; i < num_buffers; i++) {
        VkBufferCopy region = {
            .size = num_elements*num_element_bytes_array[i]
        };
        vkCmdCopyBuffer(command_buffer, stagings[i].buffer, buffers[i], 1, &region);
    }
}

void end_buffers(size_t num_buffers, const staging_t stagings[]) {
    for (size_t i = 0; i < num_buffers; i++) {
        vmaDestroyBuffer(allocator, stagings[i].buffer, stagings[i].allocation);
    }
}