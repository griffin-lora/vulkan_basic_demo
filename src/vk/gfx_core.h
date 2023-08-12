#pragma once
#include "vk.h"
#include "result.h"
#include <vk_mem_alloc.h>

// Graphics pipeline exclusive functions
result_t create_shader_module(const char* path, VkShaderModule* shader_module);
result_t create_buffer(VkDeviceSize num_buffer_bytes, VkBufferUsageFlags usage_flags, VmaAllocationCreateFlags allocation_flags, VkMemoryPropertyFlags property_flags, VkBuffer* buffer, VmaAllocation* buffer_allocation);
result_t create_mapped_buffer(VkDeviceSize num_buffer_bytes, VkBufferUsageFlags usage_flags, VmaAllocationCreateFlags allocation_flags, VkMemoryPropertyFlags property_flags, VkBuffer* buffer, VmaAllocation* buffer_allocation, void** mapped_data);
result_t write_to_staging_buffer(VmaAllocation staging_buffer_allocation, size_t num_bytes, const void* data);

void transfer_from_staging_buffer_to_buffer(VkCommandBuffer command_buffer, size_t num_bytes, VkBuffer staging_buffer, VkBuffer buffer);
void transfer_from_staging_buffer_to_image(VkCommandBuffer command_buffer, uint32_t image_width, uint32_t image_height, VkBuffer staging_buffer, VkImage image);
void transition_image_layout(VkCommandBuffer command_buffer, VkImage image, uint32_t num_mip_levels, uint32_t mip_level_index, VkImageLayout old_layout, VkImageLayout new_layout, VkAccessFlags src_access_flags, VkAccessFlags dest_access_flags, VkPipelineStageFlags src_stage_flags, VkPipelineStageFlags dest_stage_flags);

// Used by core as well
result_t create_image(uint32_t image_width, uint32_t image_height, uint32_t num_mip_levels, VkFormat format, VkSampleCountFlagBits multisample_flags, VkImageTiling tiling, VkImageUsageFlags usage_flags, VkMemoryPropertyFlags property_flags, VkImage* image, VmaAllocation* image_allocation);
result_t create_image_view(VkImage image, uint32_t num_mip_levels, VkFormat format, VkImageAspectFlags aspect_flags, VkImageView* image_view);