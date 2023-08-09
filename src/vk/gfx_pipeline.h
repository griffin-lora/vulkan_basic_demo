#pragma once
#include "vk.h"
#include "core.h"
#include <cglm/struct/mat4.h>
#include <cglm/struct/vec2.h>
#include <cglm/struct/vec3.h>

typedef struct {
    vec3s position;
    vec3s color;
} vertex_t;

extern VkDescriptorSetLayout descriptor_set_layout;
extern VkDescriptorPool descriptor_pool;
extern VkDescriptorSet descriptor_sets[NUM_FRAMES_IN_FLIGHT];

extern VkPipeline pipeline;
extern VkPipelineLayout pipeline_layout;
extern VkRenderPass render_pass;

extern VkBuffer vertex_buffer;
extern VkDeviceMemory vertex_buffer_memory;
extern VkBuffer index_buffer;
extern VkDeviceMemory index_buffer_memory;
extern VkImage texture_image;
extern VkDeviceMemory texture_image_memory;

extern VkBuffer uniform_buffers[NUM_FRAMES_IN_FLIGHT];
extern VkDeviceMemory uniform_buffers_memory[NUM_FRAMES_IN_FLIGHT];
extern void* mapped_clip_space_matrices[NUM_FRAMES_IN_FLIGHT];

extern const vertex_t vertices[4];
extern const uint16_t vertex_indices[6];

extern mat4s clip_space_matrix;

const char* init_vulkan_graphics_pipeline(void);