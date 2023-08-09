#pragma once
#include "vk.h"
#include "result.h"

// Graphics pipeline exclusive functions
VkShaderModule create_shader_module(const char* path);
uint32_t get_memory_type_index(uint32_t memory_type_bits, VkMemoryPropertyFlags property_flags);
result_t init_buffer(VkDeviceSize num_buffer_bytes, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags property_flags, VkBuffer* buffer, VkDeviceMemory* buffer_memory);
result_t write_to_staging_buffer(VkDeviceMemory staging_buffer_memory, size_t num_bytes, const void* data);

void transfer_from_staging_buffer_to_buffer(VkCommandBuffer command_buffer, size_t num_bytes, VkBuffer staging_buffer, VkBuffer buffer);
void transfer_from_staging_buffer_to_image(VkCommandBuffer command_buffer, uint32_t image_width, uint32_t image_height, VkBuffer staging_buffer, VkImage image);
void transition_image_layout(VkCommandBuffer command_buffer, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, VkAccessFlags src_access_flags, VkAccessFlags dest_access_flags, VkPipelineStageFlags src_stage_flags, VkPipelineStageFlags dest_stage_flags);

// Used by core as well
result_t init_image(uint32_t image_width, uint32_t image_height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage_flags, VkMemoryPropertyFlags property_flags, VkImage* image, VkDeviceMemory* image_memory);
result_t init_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, VkImageView* image_view);