#pragma once
#include "vk.h"
#include "result.h"
#include <vk_mem_alloc.h>
#include <stdbool.h>

// Graphics pipeline exclusive functions
result_t create_shader_module(const char* path, VkShaderModule* shader_module);
result_t write_to_buffer(VmaAllocation buffer_allocation, size_t num_bytes, const void* data);
result_t writes_to_buffer(VmaAllocation buffer_allocation, size_t num_write_bytes, size_t num_writes, const void* const data_array[]);

void transfer_from_staging_buffer_to_buffer(VkCommandBuffer command_buffer, VkDeviceSize num_bytes, VkBuffer staging_buffer, VkBuffer buffer);
void transfer_from_staging_buffer_to_image(VkCommandBuffer command_buffer, uint32_t image_width, uint32_t image_height, uint32_t num_layers, VkBuffer staging_buffer, VkImage image);
void transition_image_layout(VkCommandBuffer command_buffer, VkImage image, uint32_t num_mip_levels, uint32_t mip_level_index, uint32_t num_layers, VkImageLayout old_layout, VkImageLayout new_layout, VkAccessFlags src_access_flags, VkAccessFlags dest_access_flags, VkPipelineStageFlags src_stage_flags, VkPipelineStageFlags dest_stage_flags);

typedef struct {
    int width;
    int height;
} image_extent_t;

result_t begin_images(size_t num_images, uint32_t num_layers, const char* image_paths[][num_layers], const VkFormat formats[], image_extent_t image_extents[], uint32_t num_mip_levels_array[], VkBuffer image_staging_buffers[], VmaAllocation image_staging_allocations[], VkImage images[], VmaAllocation image_allocations[]);
void transfer_images(VkCommandBuffer command_buffer, size_t num_images, uint32_t num_layers, const image_extent_t image_extents[], const uint32_t num_mip_levels_array[], const VkBuffer image_staging_buffers[], const VkImage images[]);
void end_images(size_t num_images, const VkBuffer image_staging_buffers[], const VmaAllocation image_staging_buffer_allocations[]);
result_t create_image_views(size_t num_images, uint32_t num_layers, const VkFormat formats[], const uint32_t num_mip_levels_array[], const VkImage images[], VkImageView image_views[]);

result_t begin_vertex_arrays(
    VkDeviceSize num_vertices,
    size_t num_vertex_arrays, void* const vertex_arrays[], const VkDeviceSize num_vertex_bytes_array[], VkBuffer vertex_staging_buffers[], VmaAllocation vertex_staging_buffer_allocations[], VkBuffer vertex_buffers[], VmaAllocation vertex_buffer_allocations[]
);
void transfer_vertex_arrays(VkCommandBuffer command_buffer, VkDeviceSize num_vertices, size_t num_vertex_arrays, const VkDeviceSize num_vertex_bytes_array[], const VkBuffer vertex_staging_buffers[], const VkBuffer vertex_buffers[]);
void end_vertex_arrays(size_t num_vertex_arrays, const VkBuffer vertex_staging_buffers[], const VmaAllocation vertex_staging_buffer_allocations[]);

result_t begin_indices(VkDeviceSize num_index_bytes, VkDeviceSize num_indices, void* indices, VkBuffer* index_staging_buffer, VmaAllocation* index_staging_buffer_allocation, VkBuffer* index_buffer, VmaAllocation* index_buffer_allocation);
void transfer_indices(VkCommandBuffer command_buffer, VkDeviceSize num_index_bytes, VkDeviceSize num_indices, VkBuffer index_staging_buffer, VkBuffer index_buffer);
void end_indices(VkBuffer index_staging_buffer, VmaAllocation index_staging_buffer_allocation);

result_t begin_instances(VkDeviceSize num_instance_bytes, VkDeviceSize num_instances, const void* instances, VkBuffer* instance_staging_buffer, VmaAllocation* instance_staging_buffer_allocation, VkBuffer* instance_buffer, VmaAllocation* instance_buffer_allocation);
void transfer_instances(VkCommandBuffer command_buffer, VkDeviceSize num_instance_bytes, VkDeviceSize num_instances, VkBuffer instance_staging_buffer, VkBuffer instance_buffer);
void end_instances(VkBuffer instance_staging_buffer, VmaAllocation instance_staging_buffer_allocation);

typedef struct {
    enum {
        descriptor_info_type_buffer,
        descriptor_info_type_image
    } type;
    union {
        VkDescriptorBufferInfo buffer;
        VkDescriptorImageInfo image;
    };
} descriptor_info_t;

result_t create_descriptor_set(VkDescriptorSetLayoutCreateInfo info, descriptor_info_t infos[], VkDescriptorSetLayout* descriptor_set_layout, VkDescriptorPool* descriptor_pool, VkDescriptorSet* descriptor_set);

// Used by core as well
result_t create_image(uint32_t image_width, uint32_t image_height, uint32_t num_mip_levels, uint32_t num_layers, VkFormat format, VkSampleCountFlagBits multisample_flags, VkImageTiling tiling, VkImageUsageFlags usage_flags, VkMemoryPropertyFlags property_flags, VkImage* image, VmaAllocation* image_allocation);
result_t create_image_view(VkImage image, uint32_t num_mip_levels, uint32_t num_layers, VkFormat format, VkImageAspectFlags aspect_flags, VkImageView* image_view);
void destroy_images(size_t num_images, const VkImage images[], const VmaAllocation image_allocations[], const VkImageView image_views[]);

void begin_pipeline(
    VkCommandBuffer command_buffer,
    VkFramebuffer image_framebuffer, VkExtent2D image_extent,
    uint32_t num_clear_values, const VkClearValue clear_values[],
    VkRenderPass render_pass, VkDescriptorSet descriptor_set, VkPipelineLayout pipeline_layout, VkPipeline pipeline
);

void bind_vertex_buffers(VkCommandBuffer command_buffer, uint32_t num_vertex_buffers, const VkBuffer vertex_buffers[]);

void end_pipeline(VkCommandBuffer command_buffer);