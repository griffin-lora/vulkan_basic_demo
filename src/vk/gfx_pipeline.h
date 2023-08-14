#pragma once
#include "vk.h"
#include "core.h"
#include "mesh.h"
#include <cglm/struct/mat4.h>
#include <cglm/struct/vec2.h>
#include <cglm/struct/vec3.h>
#include <stdalign.h>
#include <assert.h>

typedef struct {
    VkRenderPass render_pass;
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
} color_pipeline_t;

extern color_pipeline_t color_pipeline;

extern VkCommandBuffer color_command_buffers[NUM_FRAMES_IN_FLIGHT];

typedef struct {
    mat4s model_view_projection;
    mat4s view;
} push_constants_t;
extern push_constants_t push_constants;
static_assert(sizeof(push_constants_t) <= 256, "Push constants must be less than or equal to 256 bytes");

const char* init_vulkan_graphics_pipelines(void);