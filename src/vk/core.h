#pragma once
#include "vk.h"
#include <stdbool.h>

#define NUM_FRAMES_IN_FLIGHT 2

typedef union {
    uint32_t data[2];
    struct {
        uint32_t graphics;
        uint32_t presentation;
    };
} queue_family_indices_t;

extern GLFWwindow* window;
extern VkDevice device;
extern VkPhysicalDevice physical_device;
extern queue_family_indices_t queue_family_indices;
extern VkSurfaceFormatKHR surface_format;
extern VkPresentModeKHR present_mode;
extern VkSemaphore image_available_semaphores[];
extern VkSemaphore render_finished_semaphores[];
extern VkFence in_flight_fences[];
extern VkCommandPool command_pool;
extern uint32_t num_swapchain_images;
extern VkImage* swapchain_images;
extern VkImageView* swapchain_image_views;
extern VkFramebuffer* swapchain_framebuffers;
extern VkSwapchainKHR swapchain;
extern VkInstance instance;
extern VkSurfaceKHR surface;
extern VkExtent2D swap_image_extent;
extern VkQueue graphics_queue;
extern VkQueue presentation_queue;
extern VkCommandBuffer command_buffers[];
extern bool framebuffer_resized;

void reinit_swapchain(void);

const char* init_vulkan_core(void);
void term_vulkan_all(void);