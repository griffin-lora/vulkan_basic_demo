#pragma once
#include "vk.h"
#include <cglm/struct/vec2.h>
#include <cglm/struct/vec3.h>

typedef struct {
    vec2s position;
    vec3s color;
} vertex_t;

extern VkPipeline pipeline;
extern VkPipelineLayout pipeline_layout;
extern VkRenderPass render_pass;

extern VkBuffer vertex_buffer;
extern VkDeviceMemory vertex_buffer_memory;
extern VkBuffer index_buffer;
extern VkDeviceMemory index_buffer_memory;

extern const vertex_t vertices[4];
extern const uint16_t vertex_indices[6];

const char* init_vulkan_graphics_pipeline(void);