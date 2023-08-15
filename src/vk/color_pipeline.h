#pragma once
#include "vk.h"
#include "core.h"
#include "result.h"
#include <vk_mem_alloc.h>
#include <cglm/struct/mat4.h>
#include <cglm/struct/vec2.h>
#include <cglm/struct/vec3.h>
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

extern VkImage color_image;
extern VmaAllocation color_image_allocation;
extern VkImageView color_image_view;

extern VkImage depth_image;
extern VmaAllocation depth_image_allocation;
extern VkImageView depth_image_view;

typedef struct {
    mat4s model_view_projection;
    mat4s view;
} color_pipeline_push_constants_t;
extern color_pipeline_push_constants_t color_pipeline_push_constants;
static_assert(sizeof(color_pipeline_push_constants_t) <= 256, "Push constants must be less than or equal to 256 bytes");

result_t init_color_pipeline_swapchain_dependents(void);
void term_color_pipeline_swapchain_dependents(void);

const char* init_color_pipeline(void);
const char* draw_color_pipeline(size_t frame_index, size_t image_index, VkCommandBuffer* out_command_buffer);
void term_color_pipeline(void);