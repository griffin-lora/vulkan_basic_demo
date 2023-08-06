#pragma once
#include "vk.h"

#define NUM_FRAMES_IN_FLIGHT 2

extern GLFWwindow* window;
extern VkDevice device;
extern VkSemaphore image_available_semaphores[];
extern VkSemaphore render_finished_semaphores[];
extern VkFence in_flight_fences[];
extern VkCommandPool command_pool;
extern uint32_t num_swapchain_images;
extern VkImageView* swapchain_image_views;
extern VkFramebuffer* swapchain_framebuffers;
extern VkSwapchainKHR swapchain;
extern VkInstance instance;
extern VkSurfaceKHR surface;
extern VkExtent2D swap_image_extent;
extern VkQueue graphics_queue;
extern VkQueue presentation_queue;
extern VkCommandBuffer command_buffers[];

const char* init_vulkan_core(void);
void term_vulkan_all(void);