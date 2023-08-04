#include "core.h"
#include "render_pass.h"
#include "gfx_pipeline.h"
#include <stdalign.h>
#include <assert.h>

alignas(64)
VkInstance instance;
VkSurfaceKHR surface;

alignas(64)
GLFWwindow* window;
VkDevice device;
VkFence in_flight_fence;
VkSwapchainKHR swapchain;
VkSemaphore image_available_semaphore;
VkCommandBuffer command_buffer;
VkRenderPass render_pass;
VkFramebuffer* swapchain_framebuffers;
static_assert(sizeof(window) + sizeof(device) + sizeof(in_flight_fence) + sizeof(swapchain) + sizeof(image_available_semaphore) + sizeof(command_buffer) + sizeof(render_pass) + sizeof(swapchain_framebuffers) == 64, "");

alignas(64)
VkExtent2D swap_image_extent;
VkPipeline pipeline;
VkSemaphore render_finished_semaphore;
VkQueue graphics_queue;
VkQueue presentation_queue;
static_assert(sizeof(swap_image_extent) + sizeof(pipeline) + sizeof(render_finished_semaphore) + sizeof(graphics_queue) + sizeof(presentation_queue) == 40, "");

VkPipelineLayout pipeline_layout;
VkCommandPool command_pool;
VkImageView* swapchain_image_views;

alignas(64) uint32_t num_swapchain_images;