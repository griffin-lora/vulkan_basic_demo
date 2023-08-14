#include "core.h"
#include "gfx_pipeline.h"
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
VkCommandBuffer render_command_buffers[NUM_FRAMES_IN_FLIGHT];
VkRenderPass render_pass;
VkFramebuffer* swapchain_framebuffers;
// static_assert(sizeof(window) + sizeof(device) + sizeof(in_flight_fence) + sizeof(swapchain) + sizeof(image_available_semaphore) + sizeof(command_buffer) + sizeof(render_pass) + sizeof(swapchain_framebuffers) == 64, "");

// alignas(64)
VkExtent2D swap_image_extent;
VkPipeline pipeline;
VkSemaphore render_finished_semaphores[NUM_FRAMES_IN_FLIGHT];
VkQueue graphics_queue;
VkQueue presentation_queue;
// static_assert(sizeof(swap_image_extent) + sizeof(pipeline) + sizeof(render_finished_semaphore) + sizeof(graphics_queue) + sizeof(presentation_queue) == 40, "");

VkPipelineLayout pipeline_layout;
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

VkBuffer vertex_buffers[NUM_VERTEX_ARRAYS];
VmaAllocation vertex_buffer_allocations[NUM_VERTEX_ARRAYS];

VkBuffer index_buffer;
VmaAllocation index_buffer_allocation;

VkSampler world_texture_image_sampler;
VkImage texture_images[NUM_TEXTURE_IMAGES];
VmaAllocation texture_image_allocations[NUM_TEXTURE_IMAGES];
VkImageView texture_image_views[NUM_TEXTURE_IMAGES];

size_t num_indices;
push_constants_t push_constants;

VkDescriptorSetLayout descriptor_set_layout;
VkDescriptorPool descriptor_pool;
VkDescriptorSet descriptor_set;

VkFormat depth_image_format;
VkImage depth_image;
VmaAllocation depth_image_allocation;
VkImageView depth_image_view;

VkImage color_image;
VmaAllocation color_image_allocation;
VkImageView color_image_view;

VkSampleCountFlagBits render_multisample_flags;