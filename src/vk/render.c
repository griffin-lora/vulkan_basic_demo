#include "render.h"
#include "core.h"
#include "gfx_pipeline.h"
#include "color_pipeline.h"
#include "gfx_core.h"
#include "asset.h"
#include "result.h"
#include "util.h"
#include <string.h>

static uint32_t frame_index = 0;

const char* draw_vulkan_frame(void) {
    VkSemaphore image_available_semaphore = image_available_semaphores[frame_index];
    VkSemaphore render_finished_semaphore = render_finished_semaphores[frame_index];
    VkFence in_flight_fence = in_flight_fences[frame_index];

    vkWaitForFences(device, 1, &in_flight_fence, VK_TRUE, UINT64_MAX);

    uint32_t image_index;
    {
        VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_available_semaphore, VK_NULL_HANDLE, &image_index);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            reinit_swapchain();
            return NULL;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            return "Failed to acquire swapchain image";
        }
    }

    vkResetFences(device, 1, &in_flight_fence);

    VkCommandBuffer command_buffer;
    const char* msg = draw_color_pipeline(frame_index, image_index, &command_buffer);
    if (msg != NULL) {
        return msg;
    }

    VkPipelineStageFlags wait_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    if (vkQueueSubmit(graphics_queue, 1, &(VkSubmitInfo) {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &image_available_semaphore,
        .pWaitDstStageMask = &wait_stage_flags,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &render_finished_semaphore
    }, in_flight_fence) != VK_SUCCESS) {
        return "Failed to submit to graphics queue\n";
    }

    {
        VkResult result = vkQueuePresentKHR(presentation_queue, &(VkPresentInfoKHR) {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_finished_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &swapchain,
            .pImageIndices = &image_index
        });
        
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized) {
            framebuffer_resized = false;
            vkWaitForFences(device, 1, &in_flight_fence, VK_TRUE, UINT64_MAX);
            reinit_swapchain();
        } else if (result != VK_SUCCESS) {
            return "Failed to present swap chain image";
        }
    }

    frame_index += 1;
    frame_index %= NUM_FRAMES_IN_FLIGHT;

    return NULL;
}