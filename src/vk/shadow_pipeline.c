#include "shadow_pipeline.h"
#include "core.h"
#include "gfx_core.h"
#include "asset.h"
#include "util.h"
#include "mesh.h"
#include <vk_mem_alloc.h>
#include <cglm/struct/mat4.h>
#include <cglm/struct/cam.h>
#include <cglm/struct/mat3.h>
#include <cglm/struct/affine.h>

static struct {
    VkRenderPass render_pass;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
} shadow_pipeline;

static struct {
    mat4s model_view_projection;
} shadow_pipeline_push_constants;

#define SHADOW_IMAGE_SIZE 512

static VkImage shadow_image;
static VmaAllocation shadow_image_allocation;
VkImageView shadow_image_view;

const char* init_shadow_pipeline(void) {
    if (create_image(SHADOW_IMAGE_SIZE, SHADOW_IMAGE_SIZE, 1, depth_image_format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &shadow_image, &shadow_image_allocation) != result_success) {
        return "Failed to create shadow image\n";
    }

    if (create_image_view(shadow_image, 1, depth_image_format, depth_image_format == VK_FORMAT_D32_SFLOAT_S8_UINT || depth_image_format == VK_FORMAT_D24_UNORM_S8_UINT ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_DEPTH_BIT, &shadow_image_view) != result_success) {
        return "Failed to create shadow image view\n";
    }

    VkAttachmentReference depth_attachment_reference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 0,
        .pDepthStencilAttachment = &depth_attachment_reference
    };

    VkAttachmentDescription attachments[] = {
        {
            .format = depth_image_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        }
    };

    {
        VkRenderPassCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = NUM_ELEMS(attachments),
            .pAttachments = attachments,
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 0
        };

        if (vkCreateRenderPass(device, &info, NULL, &shadow_pipeline.render_pass) != VK_SUCCESS) {
            return "Failed to create render pass\n";
        }
    }

    VkShaderModule vertex_shader_module;
    if (create_shader_module("shader/shadow_pipeline_vertex.spv", &vertex_shader_module) != result_success) {
        return "Failed to create vertex shader module\n";
    }

    shader_t shaders[] = {
        {
            .stage_flags = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertex_shader_module
        }
    };
    
    uint32_t num_pass_vertex_bytes_array[] = {
        num_vertex_bytes_array[GENERAL_PIPELINE_VERTEX_ARRAY_INDEX]
    };

    vertex_attribute_t attributes[] = {
        {
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(general_pipeline_vertex_t, position)
        }
    };

    const char* msg = create_graphics_pipeline(
        NUM_ELEMS(shaders), shaders,
        0, NULL, NULL,
        NUM_ELEMS(num_pass_vertex_bytes_array), num_pass_vertex_bytes_array,
        NUM_ELEMS(attributes), attributes,
        sizeof(shadow_pipeline_push_constants),
        VK_SAMPLE_COUNT_1_BIT,
        shadow_pipeline.render_pass,
        NULL, NULL, NULL, &shadow_pipeline.pipeline_layout, &shadow_pipeline.pipeline
    );
    if (msg != NULL) {
        return msg;
    }

    vkDestroyShaderModule(device, vertex_shader_module, NULL);

    mat4s projection = glms_perspective((GLM_PI*2.0f) / 5.0f, 1, 0.01f, 300.0f);
    mat4s view = glms_look((vec3s) {{ 5.5f, 4.0f, -6.0f }}, (vec3s) {{ -0.5f, -0.5f, 0.5f }}, (vec3s) {{ 0.0f, -1.0f, 0.0f }});

    shadow_pipeline_push_constants.model_view_projection = glms_mat4_mul(projection, view);

    return NULL;
}

const char* draw_shadow_pipeline(void) {
    VkCommandBuffer command_buffer;
    {
        VkCommandBufferAllocateInfo info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandPool = command_pool, // TODO: Use separate command pool
            .commandBufferCount = 1
        };

        if (vkAllocateCommandBuffers(device, &info, &command_buffer) != VK_SUCCESS) {
            return "Failed to create command buffer\n";
        }
    }

    {
        VkCommandBufferBeginInfo info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };
        if (vkBeginCommandBuffer(command_buffer, &info) != VK_SUCCESS) {
            return "Failed to write to command buffer\n";
        }
    }

    VkFramebuffer framebuffer;
    {
        VkImageView attachments[] = { shadow_image_view };

        VkFramebufferCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = shadow_pipeline.render_pass,
            .attachmentCount = NUM_ELEMS(attachments),
            .pAttachments = attachments,
            .width = SHADOW_IMAGE_SIZE,
            .height = SHADOW_IMAGE_SIZE,
            .layers = 1
        };

        if (vkCreateFramebuffer(device, &info, NULL, &framebuffer) != VK_SUCCESS) {
            return "Failed to create shadow image framebuffer\n";
        }
    }

    VkClearValue clear_values[] = {
        { .depthStencil = { .depth = 1.0f, .stencil = 0 } },
    };

    VkBuffer pass_vertex_buffers[] = {
        vertex_buffers[GENERAL_PIPELINE_VERTEX_ARRAY_INDEX]
    };

    draw_scene(
        command_buffer,
        framebuffer, (VkExtent2D) { .width = SHADOW_IMAGE_SIZE, .height = SHADOW_IMAGE_SIZE },
        NUM_ELEMS(clear_values), clear_values,
        shadow_pipeline.render_pass, NULL, shadow_pipeline.pipeline_layout, shadow_pipeline.pipeline,
        sizeof(shadow_pipeline_push_constants), &shadow_pipeline_push_constants,
        NUM_ELEMS(pass_vertex_buffers), pass_vertex_buffers,
        num_indices, index_buffer
    );

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        return "Failed to write to transfer command buffer\n";
    }

    {
        VkSubmitInfo info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffer
        };

        vkQueueSubmit(graphics_queue, 1, &info, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphics_queue);
    }

    vkDestroyFramebuffer(device, framebuffer, NULL);

    return NULL;
}

void term_shadow_pipeline(void) {
    vkDestroyPipeline(device, shadow_pipeline.pipeline, NULL);
    vkDestroyPipelineLayout(device, shadow_pipeline.pipeline_layout, NULL);
    vkDestroyRenderPass(device, shadow_pipeline.render_pass, NULL);

    vkDestroyImageView(device, shadow_image_view, NULL);
    vmaDestroyImage(allocator, shadow_image, shadow_image_allocation);
}