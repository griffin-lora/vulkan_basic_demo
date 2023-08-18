#pragma once
#include "vk.h"
#include "mesh.h"
#include <vk_mem_alloc.h>
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/struct/mat4.h>

#define NUM_MODELS 2
extern VkBuffer vertex_buffer_arrays[NUM_MODELS][NUM_VERTEX_ARRAYS];
extern VmaAllocation vertex_buffer_allocation_arrays[NUM_MODELS][NUM_VERTEX_ARRAYS];

extern VkBuffer index_buffers[NUM_MODELS];
extern VmaAllocation index_buffer_allocations[NUM_MODELS];

extern VkBuffer instance_buffers[NUM_MODELS];
extern VmaAllocation instance_buffer_allocations[NUM_MODELS];

extern uint32_t num_indices_array[NUM_MODELS];
extern uint32_t num_instances_array[NUM_MODELS];

#define NUM_TEXTURE_IMAGES 3
#define NUM_TEXTURE_LAYERS 1
extern VkSampler texture_image_sampler;
extern VkImage texture_images[NUM_TEXTURE_IMAGES];
extern VmaAllocation texture_image_allocations[NUM_TEXTURE_IMAGES];
extern VkImageView texture_image_views[NUM_TEXTURE_IMAGES];

extern VkSampler shadow_texture_image_sampler;
extern mat4s shadow_view_projection;
extern VkBuffer shadow_view_projection_buffer;
extern VmaAllocation shadow_view_projection_buffer_allocation;

const char* init_vulkan_assets(const VkPhysicalDeviceProperties* physical_device_properties);
void term_vulkan_assets(void);