#pragma once
#include <vulkan/vulkan.h>

extern VkImageView shadow_image_view;

const char* init_shadow_pipeline(void);
const char* draw_shadow_pipeline(void);
void term_shadow_pipeline(void);