#pragma once
#include "vk.h"

extern VkPipeline pipeline;
extern VkPipelineLayout pipeline_layout;
extern VkRenderPass render_pass;

const char* init_vulkan_graphics_pipeline(VkFormat surface_format);