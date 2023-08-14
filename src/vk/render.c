#include "render.h"
#include "core.h"
#include "gfx_pipeline.h"
#include "gfx_core.h"
#include "asset.h"
#include "result.h"
#include "util.h"
#include <string.h>

static uint32_t frame_index = 0;

const char* draw_vulkan_frame(void) {
    VkSemaphore image_available_semaphore = image_available_semaphores[frame_index];
    VkSemaphore render_finished_semaphore = render_finished_semaphores[frame_index];
    VkCommandBuffer command_buffer = render_command_buffer_array[COLOR_PIPELINE_INDEX][frame_index];
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

    vkResetCommandBuffer(command_buffer, 0);
    {
        VkCommandBufferBeginInfo info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
        };

        if (vkBeginCommandBuffer(command_buffer, &info) != VK_SUCCESS) {
            return "Failed to begin writing to command buffer\n";
        }
    }

    VkClearValue clear_values[] = {
        { .color = { .float32 = { 0.0f, 0.0f, 0.0f, 1.0f } } },
        { .depthStencil = { .depth = 1.0f, .stencil = 0 } },
    };

    VkBuffer pass_vertex_buffers[] = {
        vertex_buffers[GENERAL_PASS_VERTEX_ARRAY_INDEX],
        vertex_buffers[COLOR_PASS_VERTEX_ARRAY_INDEX]
    };

    draw_scene(
        command_buffer,
        swapchain_framebuffers[image_index], swap_image_extent,
        NUM_ELEMS(clear_values), clear_values,
        render_passes[COLOR_PIPELINE_INDEX], descriptor_sets[COLOR_PIPELINE_INDEX], pipeline_layouts[COLOR_PIPELINE_INDEX], pipelines[COLOR_PIPELINE_INDEX],
        sizeof(push_constants), &push_constants,
        NUM_ELEMS(pass_vertex_buffers), pass_vertex_buffers,
        num_indices, index_buffer
    );

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        return "Failed to end command buffer\n";
    }

    {
        VkPipelineStageFlags wait_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &image_available_semaphore,
            .pWaitDstStageMask = &wait_stage_flags,
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &render_finished_semaphore
        };

        if (vkQueueSubmit(graphics_queue, 1, &info, in_flight_fence) != VK_SUCCESS) {
            return "Failed to submit to graphics queue\n";
        }
    }

    {
        VkPresentInfoKHR info = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_finished_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &swapchain,
            .pImageIndices = &image_index
        };

        VkResult result = vkQueuePresentKHR(presentation_queue, &info);
        
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized) {
            framebuffer_resized = false;
            reinit_swapchain();
        } else if (result != VK_SUCCESS) {
            return "Failed to present swap chain image";
        }
    }

    frame_index += 1;
    frame_index %= NUM_FRAMES_IN_FLIGHT;

    return NULL;
}