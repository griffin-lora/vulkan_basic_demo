#include "core.h"
#include "gfx_pipeline.h"
#include "shadow_pipeline.h"
#include "asset.h"
#include <stdalign.h>
#include <assert.h>

// TODO: Fix this mess

alignas(64)
VkInstance instance;
VkSurfaceKHR surface;

alignas(64)
GLFWwindow* window;
VkDevice device;
VmaAllocator allocator;
VkFence in_flight_fences[NUM_FRAMES_IN_FLIGHT];
VkSwapchainKHR swapchain;
VkSemaphore image_available_semaphores[NUM_FRAMES_IN_FLIGHT];
VkFramebuffer* swapchain_framebuffers;
// static_assert(sizeof(window) + sizeof(device) + sizeof(in_flight_fence) + sizeof(swapchain) + sizeof(image_available_semaphore) + sizeof(command_buffer) + sizeof(render_pass) + sizeof(swapchain_framebuffers) == 64, "");

// alignas(64)
VkExtent2D swap_image_extent;
VkSemaphore render_finished_semaphores[NUM_FRAMES_IN_FLIGHT];
VkQueue graphics_queue;
VkQueue presentation_queue;
// static_assert(sizeof(swap_image_extent) + sizeof(pipeline) + sizeof(render_finished_semaphore) + sizeof(graphics_queue) + sizeof(presentation_queue) == 40, "");

VkCommandPool command_pool;
VkImage* swapchain_images;
VkImageView* swapchain_image_views;

// alignas(64)
uint32_t num_swapchain_images;

VkPhysicalDevice physical_device;
queue_family_indices_t queue_family_indices;
VkSurfaceFormatKHR surface_format;
VkPresentModeKHR present_mode;
bool framebuffer_resized = false;

VkSampler texture_image_sampler;
VkImage texture_images[NUM_TEXTURE_IMAGES];
VmaAllocation texture_image_allocations[NUM_TEXTURE_IMAGES];
VkImageView texture_image_views[NUM_TEXTURE_IMAGES];

VkFormat depth_image_format;

VkSampleCountFlagBits render_multisample_flags;

VkBuffer shadow_view_projection_buffer;
VmaAllocation shadow_view_projection_buffer_allocation;
VkSampler shadow_texture_image_sampler;


VkBuffer vertex_buffer_arrays[NUM_MODELS][NUM_VERTEX_ARRAYS];
VmaAllocation vertex_buffer_allocation_arrays[NUM_MODELS][NUM_VERTEX_ARRAYS];

VkBuffer index_buffers[NUM_MODELS];
VmaAllocation index_buffer_allocations[NUM_MODELS];

VkBuffer instance_buffers[NUM_MODELS];
VmaAllocation instance_buffer_allocations[NUM_MODELS];

uint32_t num_indices_array[NUM_MODELS];
uint32_t num_instances_array[NUM_MODELS];