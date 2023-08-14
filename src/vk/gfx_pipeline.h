#pragma once
#include "vk.h"
#include "core.h"
#include "mesh.h"
#include <cglm/struct/mat4.h>
#include <cglm/struct/vec2.h>
#include <cglm/struct/vec3.h>
#include <stdalign.h>
#include <assert.h>

#define NUM_PIPELINES 2
#define SHADOW_PIPELINE_INDEX 0
#define COLOR_PIPELINE_INDEX 1

extern VkRenderPass render_passes[NUM_PIPELINES];
extern VkDescriptorSetLayout descriptor_set_layouts[NUM_PIPELINES];
extern VkDescriptorPool descriptor_pools[NUM_PIPELINES];
extern VkDescriptorSet descriptor_sets[NUM_PIPELINES];
extern VkPipelineLayout pipeline_layouts[NUM_PIPELINES];
extern VkPipeline pipelines[NUM_PIPELINES];

typedef struct {
    mat4s model_view_projection;
    mat4s view;
} push_constants_t;
extern push_constants_t push_constants;
static_assert(sizeof(push_constants_t) <= 256, "Push constants must be less than or equal to 256 bytes");

const char* init_vulkan_graphics_pipelines(void);