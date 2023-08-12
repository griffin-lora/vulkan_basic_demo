#pragma once
#include "vk.h"
#include "core.h"
#include "mesh.h"
#include <cglm/struct/mat4.h>
#include <cglm/struct/vec2.h>
#include <cglm/struct/vec3.h>

extern VkDescriptorSetLayout descriptor_set_layout;
extern VkDescriptorPool descriptor_pool;
extern VkDescriptorSet descriptor_sets[NUM_FRAMES_IN_FLIGHT];

extern VkPipeline pipeline;
extern VkPipelineLayout pipeline_layout;
extern VkRenderPass render_pass;

extern VkBuffer vertex_buffer;
extern VmaAllocation vertex_buffer_allocation;
extern VkBuffer index_buffer;
extern VmaAllocation index_buffer_allocation;
extern VkImage texture_image;
extern VmaAllocation texture_image_allocation;

extern size_t num_indices;
extern mat4s clip_space;

extern VkImageView texture_image_view;
extern VkSampler texture_image_sampler;

const char* init_vulkan_graphics_pipeline(const VkPhysicalDeviceProperties* physical_device_properties);