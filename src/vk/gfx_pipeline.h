#pragma once
#include "vk.h"
#include "core.h"
#include "mesh.h"
#include <cglm/struct/mat4.h>
#include <cglm/struct/vec2.h>
#include <cglm/struct/vec3.h>

extern VkDescriptorSetLayout descriptor_set_layout;
extern VkDescriptorPool descriptor_pool;
extern VkDescriptorSet descriptor_set;

extern VkPipeline pipeline;
extern VkPipelineLayout pipeline_layout;
extern VkRenderPass render_pass;

extern VkBuffer vertex_buffer;
extern VmaAllocation vertex_buffer_allocation;
extern VkBuffer index_buffer;
extern VmaAllocation index_buffer_allocation;

extern size_t num_indices;
extern mat4s clip_space;

#define NUM_WORLD_TEXTURE_IMAGES 2
extern VkSampler world_texture_image_sampler;
extern VkImage world_texture_images[NUM_WORLD_TEXTURE_IMAGES];
extern VmaAllocation world_texture_image_allocations[NUM_WORLD_TEXTURE_IMAGES];
extern VkImageView world_texture_image_views[NUM_WORLD_TEXTURE_IMAGES];

const char* init_vulkan_graphics_pipeline(const VkPhysicalDeviceProperties* physical_device_properties);