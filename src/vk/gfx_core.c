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

uint32_t get_memory_type_index(uint32_t memory_type_bits, VkMemoryPropertyFlags property_flags) {
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
result_t create_buffer(VkDeviceSize num_buffer_bytes, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags property_flags, VkBuffer* buffer, VkDeviceMemory* buffer_memory) {
    {
        VkBufferCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = num_buffer_bytes,
            .usage = usage_flags,
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

result_t write_to_staging_buffer(VkDeviceMemory staging_buffer_memory, size_t num_bytes, const void* data) {
    void* mapped_data;
    if (vkMapMemory(device, staging_buffer_memory, 0, num_bytes, 0, &mapped_data) != VK_SUCCESS) {
        return result_failure;
    }
    memcpy(mapped_data, data, num_bytes);
    vkUnmapMemory(device, staging_buffer_memory);

    return result_success;
}

result_t create_image(uint32_t image_width, uint32_t image_height, uint32_t num_mip_levels, VkFormat format, VkSampleCountFlagBits multisample_flags, VkImageTiling tiling, VkImageUsageFlags usage_flags, VkMemoryPropertyFlags property_flags, VkImage* image, VkDeviceMemory* image_memory) {
    {
        VkImageCreateInfo info = {
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

        if (vkCreateImage(device, &info, NULL, image) != VK_SUCCESS) {
            return result_failure;
        }
    }

    {
        VkMemoryRequirements requirements;
        vkGetImageMemoryRequirements(device, *image, &requirements);

        VkMemoryAllocateInfo info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = requirements.size,
            .memoryTypeIndex = get_memory_type_index(requirements.memoryTypeBits, property_flags)
        };

        if (vkAllocateMemory(device, &info, NULL, image_memory) != VK_SUCCESS) {
            return result_failure;
        }
    }

    if (vkBindImageMemory(device, *image, *image_memory, 0) != VK_SUCCESS) {
        return result_failure;
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