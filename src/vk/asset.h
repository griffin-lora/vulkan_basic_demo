#pragma once
#include "vk.h"
#include "mesh.h"
#include <vk_mem_alloc.h>

extern VkBuffer vertex_buffers[NUM_VERTEX_ARRAYS];
extern VmaAllocation vertex_buffer_allocations[NUM_VERTEX_ARRAYS];

extern VkBuffer index_buffer;
extern VmaAllocation index_buffer_allocation;

extern size_t num_indices;

#define NUM_TEXTURE_IMAGES 2
extern VkSampler world_texture_image_sampler;
extern VkImage texture_images[NUM_TEXTURE_IMAGES];
extern VmaAllocation texture_image_allocations[NUM_TEXTURE_IMAGES];
extern VkImageView texture_image_views[NUM_TEXTURE_IMAGES];

const char* init_vulkan_assets(const VkPhysicalDeviceProperties* physical_device_properties);