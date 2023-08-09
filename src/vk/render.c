#include "render.h"
#include "core.h"
#include "gfx_pipeline.h"
#include "result.h"
#include "util.h"
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/struct/cam.h>
#include <string.h>

#define M_TAU (GLM_PI * 2)

static uint32_t frame_index = 0;

const char* draw_vulkan_frame(void) {
    VkSemaphore image_available_semaphore = image_available_semaphores[frame_index];
    VkSemaphore render_finished_semaphore = render_finished_semaphores[frame_index];
    VkCommandBuffer command_buffer = render_command_buffers[frame_index];
    VkFence in_flight_fence = in_flight_fences[frame_index];

    frame_index += 1;
    frame_index %= NUM_FRAMES_IN_FLIGHT;

    vkWaitForFences(device, 1, &in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &in_flight_fence);

    uint32_t image_index;
    {
        VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_available_semaphore, VK_NULL_HANDLE, &image_index);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized) {
            reinit_swapchain();
            vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_available_semaphore, VK_NULL_HANDLE, &image_index);
        } else if (result != VK_SUCCESS) {
            return "Failed to acquire swapchain image";
        }
    }

    {   
        mat4s projection = glms_perspective(M_TAU / 5.0f, (float)swap_image_extent.width/(float)swap_image_extent.height, 0.01f, 300.0f);
        mat4s view = glms_look((vec3s){{ -0.5f, 1.0f, 1.0f }}, (vec3s){{ 0.5f, -1.0f, -1.0f }}, (vec3s){{ 0.0f, 1.0f, 0.0f }});

        clip_space = glms_mat4_mul(projection, view);

        memcpy(mapped_clip_spaces[frame_index], &clip_space, sizeof(clip_space));
    }

    vkResetCommandBuffer(command_buffer, 0);
    // write to command buffer
    {
        VkCommandBufferBeginInfo info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
        };

        if (vkBeginCommandBuffer(command_buffer, &info) != VK_SUCCESS) {
            return "Failed to begin to write to command buffer";
        }
    }

    VkClearValue clear_color = {{{ 0.0f, 0.0f, 0.0f, 1.0f }}};

    {
        VkRenderPassBeginInfo info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = render_pass,
            .framebuffer = swapchain_framebuffers[image_index],
            .renderArea.offset = { 0, 0 },
            .renderArea.extent = swap_image_extent,
            .clearValueCount = 1,
            .pClearValues = &clear_color
        };

        vkCmdBeginRenderPass(command_buffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offset);

    vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets[frame_index], 0, NULL);

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = swap_image_extent.width,
        .height = swap_image_extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = { 0, 0 },
        .extent = swap_image_extent
    };
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    vkCmdDrawIndexed(command_buffer, NUM_ELEMS(vertex_indices), 1, 0, 0, 0);

    vkCmdEndRenderPass(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        return "Failed to write to command buffer";
    }

    //

    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        
        VkSubmitInfo info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &image_available_semaphore,
            .pWaitDstStageMask = &wait_stage,
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &render_finished_semaphore
        };

        if (vkQueueSubmit(graphics_queue, 1, &info, in_flight_fence) != VK_SUCCESS) {
            return "Failed to submit draw command buffer\n";
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
        
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            reinit_swapchain();
        } else if (result != VK_SUCCESS) {
            return "Failed to present swap chain image";
        }
    }

    return NULL;
}