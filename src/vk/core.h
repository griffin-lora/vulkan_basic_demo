#pragma once
#include "vk.h"
#include <stdbool.h>
#include <vk_mem_alloc.h>

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
extern VmaAllocator allocator;
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
extern VkCommandBuffer render_command_buffers[];
extern bool framebuffer_resized;

extern VkSampleCountFlagBits render_multisample_flags;

extern VkImage color_image;
extern VmaAllocation color_image_allocation;
extern VkImageView color_image_view;

extern VkFormat depth_image_format;
extern VkImage depth_image;
extern VmaAllocation depth_image_allocation;
extern VkImageView depth_image_view;


void reinit_swapchain(void);

const char* init_vulkan_core(void);
void term_vulkan_all(void);