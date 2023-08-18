#pragma once
#include "vk.h"
#include "core.h"
#include "result.h"
#include <vk_mem_alloc.h>
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/struct/mat4.h>
#include <cglm/struct/vec2.h>
#include <cglm/struct/vec3.h>
#include <assert.h>

extern VkRenderPass color_pipeline_render_pass;

extern VkImageView color_image_view;

extern VkImageView depth_image_view;

typedef struct {
    mat4s view_projection;
    vec3s camera_position;
    float layer_index;
} color_pipeline_push_constants_t;
extern color_pipeline_push_constants_t color_pipeline_push_constants;
static_assert(sizeof(color_pipeline_push_constants_t) <= 256, "Push constants must be less than or equal to 256 bytes");

result_t init_color_pipeline_swapchain_dependents(void);
void term_color_pipeline_swapchain_dependents(void);

const char* init_color_pipeline(void);
const char* draw_color_pipeline(size_t frame_index, size_t image_index, VkCommandBuffer* out_command_buffer);
void term_color_pipeline(void);