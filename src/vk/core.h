#pragma once
#include "vk.h"

extern GLFWwindow* window;
extern VkDevice device;
extern VkSemaphore image_available_semaphore;
extern VkSemaphore render_finished_semaphore;
extern VkFence in_flight_fence;
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
extern VkCommandBuffer command_buffer;

const char* init_vulkan_core(void);
void term_vulkan_all(void);