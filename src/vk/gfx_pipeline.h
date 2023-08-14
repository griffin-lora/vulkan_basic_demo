#pragma once
#include "vk.h"
#include "core.h"
#include "mesh.h"
#include <cglm/struct/mat4.h>
#include <cglm/struct/vec2.h>
#include <cglm/struct/vec3.h>
#include <stdalign.h>
#include <assert.h>

extern VkDescriptorSetLayout descriptor_set_layout;
extern VkDescriptorPool descriptor_pool;
extern VkDescriptorSet descriptor_set;

extern VkPipeline pipeline;
extern VkPipelineLayout pipeline_layout;
extern VkRenderPass render_pass;

extern VkBuffer vertex_buffers[NUM_VERTEX_ARRAYS];
extern VmaAllocation vertex_buffer_allocations[NUM_VERTEX_ARRAYS];

extern VkBuffer index_buffer;
extern VmaAllocation index_buffer_allocation;

extern size_t num_indices;
typedef struct {
    mat4s model_view_projection;
    mat4s view;
} push_constants_t;
extern push_constants_t push_constants;
static_assert(sizeof(push_constants_t) <= 256, "Push constants must be less than or equal to 256 bytes");

#define NUM_TEXTURE_IMAGES 2
extern VkSampler world_texture_image_sampler;
extern VkImage texture_images[NUM_TEXTURE_IMAGES];
extern VmaAllocation texture_image_allocations[NUM_TEXTURE_IMAGES];
extern VkImageView texture_image_views[NUM_TEXTURE_IMAGES];

const char* init_vulkan_graphics_pipeline(const VkPhysicalDeviceProperties* physical_device_properties);