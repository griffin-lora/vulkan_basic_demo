#include "shadow_pipeline.h"
#include "core.h"
#include "gfx_core.h"
#include "asset.h"
#include "util.h"
#include "mesh.h"
#include "defaults.h"
#include <vk_mem_alloc.h>
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/struct/mat4.h>
#include <cglm/struct/cam.h>
#include <cglm/struct/mat3.h>
#include <cglm/struct/affine.h>

static VkRenderPass render_pass;
static VkDescriptorSetLayout descriptor_set_layout;
static VkDescriptorPool descriptor_pool;
static VkDescriptorSet descriptor_set;
static VkPipelineLayout pipeline_layout;
static VkPipeline pipeline;

#define SHADOW_IMAGE_SIZE 4096

static VkImage shadow_image;
static VmaAllocation shadow_image_allocation;
VkImageView shadow_image_view;

const char* init_shadow_pipeline(void) {
    if (create_image(SHADOW_IMAGE_SIZE, SHADOW_IMAGE_SIZE, 1, 1, depth_image_format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &shadow_image, &shadow_image_allocation) != result_success) {
        return "Failed to create shadow image\n";
    }

    if (create_image_view(shadow_image, 1, 1, depth_image_format, depth_image_format == VK_FORMAT_D32_SFLOAT_S8_UINT || depth_image_format == VK_FORMAT_D24_UNORM_S8_UINT ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_DEPTH_BIT, &shadow_image_view) != result_success) {
        return "Failed to create shadow image view\n";
    }

    VkAttachmentReference depth_attachment_reference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .pDepthStencilAttachment = &depth_attachment_reference
    };

    VkAttachmentDescription attachments[] = {
        {
            DEFAULT_VK_ATTACHMENT,
            .format = depth_image_format,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        }
    };

    {
        VkRenderPassCreateInfo info = {
            DEFAULT_VK_RENDER_PASS,
            .attachmentCount = NUM_ELEMS(attachments),
            .pAttachments = attachments,
            .pSubpasses = &subpass
        };

        if (vkCreateRenderPass(device, &info, NULL, &render_pass) != VK_SUCCESS) {
            return "Failed to create render pass\n";
        }
    }

    VkDescriptorSetLayoutBinding descriptor_bindings[] = {
        {
            DEFAULT_VK_DESCRIPTOR_BINDING,
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
        }
    };

    descriptor_info_t descriptor_infos[] = {
        {
            .type = descriptor_info_type_buffer,
            .buffer = {
                .buffer = shadow_view_projection_buffer,
                .offset = 0,
                .range = sizeof(shadow_view_projection)
            }
        }
    };

    if (create_descriptor_set(
        (VkDescriptorSetLayoutCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = NUM_ELEMS(descriptor_bindings),
            .pBindings = descriptor_bindings
        }, descriptor_infos,
        &descriptor_set_layout, &descriptor_pool, &descriptor_set
    ) != result_success) {
        return "Failed to create descriptor set\n";
    }

    {
        VkPipelineLayoutCreateInfo info = {
            DEFAULT_VK_PIPELINE_LAYOUT,
            .pSetLayouts = &descriptor_set_layout,
        };

        if (vkCreatePipelineLayout(device, &info, NULL, &pipeline_layout) != VK_SUCCESS) {
            return "Failed to create pipeline layout\n";
        }
    }

    VkShaderModule vertex_shader_module;
    if (create_shader_module("shader/shadow_pipeline_vertex.spv", &vertex_shader_module) != result_success) {
        return "Failed to create vertex shader module\n";
    }

    VkPipelineShaderStageCreateInfo shader_infos[] = {
        {
            DEFAULT_VK_SHADER_STAGE,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertex_shader_module
        }
    };
    
    VkVertexInputBindingDescription vertex_bindings[] = {
        {
            .binding = 0,
            .stride = sizeof(mat4s),
            .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
        },
        {
            .binding = 1,
            .stride = num_vertex_bytes_array[GENERAL_PIPELINE_VERTEX_ARRAY_INDEX],
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        }
    };

    VkVertexInputAttributeDescription vertex_attributes[] = {
        {
            .binding = 0,
            .location = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = 0*sizeof(vec4s)
        },
        {
            .binding = 0,
            .location = 1,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = 1*sizeof(vec4s)
        },
        {
            .binding = 0,
            .location = 2,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = 2*sizeof(vec4s)
        },
        {
            .binding = 0,
            .location = 3,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = 3*sizeof(vec4s)
        },
        //
        {
            .binding = 1,
            .location = 4,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(general_pipeline_vertex_t, position)
        }
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = NUM_ELEMS(vertex_bindings),
        .pVertexBindingDescriptions = vertex_bindings,
        .vertexAttributeDescriptionCount = NUM_ELEMS(vertex_attributes),
        .pVertexAttributeDescriptions = vertex_attributes
    };

    VkPipelineRasterizationStateCreateInfo rasterization_info = {
        DEFAULT_VK_RASTERIZATION,
        .depthBiasEnable = VK_TRUE,
        .depthBiasConstantFactor = 4.0f,
        .depthBiasSlopeFactor = 1.5f
    };

    VkPipelineMultisampleStateCreateInfo multisample_info = { DEFAULT_VK_MULTISAMPLE };

    VkGraphicsPipelineCreateInfo info = {
        DEFAULT_VK_GRAPHICS_PIPELINE,
        .stageCount = NUM_ELEMS(shader_infos),
        .pStages = shader_infos,
        .pVertexInputState = &vertex_input_info,
        .pRasterizationState = &rasterization_info,
        .pMultisampleState = &multisample_info,
        .layout = pipeline_layout,
        .renderPass = render_pass
    };

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, NULL, &pipeline) != VK_SUCCESS) {
        return "Failed to create graphics pipeline\n";
    }

    vkDestroyShaderModule(device, vertex_shader_module, NULL);

    return NULL;
}

const char* draw_shadow_pipeline(void) {
    VkCommandBuffer command_buffer;
    {
        VkCommandBufferAllocateInfo info = {
            DEFAULT_VK_COMMAND_BUFFER,
            .commandPool = command_pool // TODO: Use separate command pool
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
            .renderPass = render_pass,
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

    begin_pipeline(
        command_buffer,
        framebuffer, (VkExtent2D) { .width = SHADOW_IMAGE_SIZE, .height = SHADOW_IMAGE_SIZE },
        NUM_ELEMS(clear_values), clear_values,
        render_pass, descriptor_set, pipeline_layout, pipeline
    );

    for (size_t i = 0; i < NUM_MODELS; i++) {
        VkBuffer pass_vertex_buffers[] = {
            instance_buffers[i],
            vertex_buffer_arrays[i][GENERAL_PIPELINE_VERTEX_ARRAY_INDEX]
        };

        draw_instanced_model(
            command_buffer,
            NUM_ELEMS(pass_vertex_buffers), pass_vertex_buffers,
            num_indices_array[i], index_buffers[i],
            num_instances_array[i]
        );
    }

    end_pipeline(command_buffer);

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
    vkDestroyPipeline(device, pipeline, NULL);
    vkDestroyPipelineLayout(device, pipeline_layout, NULL);
    vkDestroyRenderPass(device, render_pass, NULL);
    vkDestroyDescriptorPool(device, descriptor_pool, NULL);
    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, NULL);


    vkDestroyImageView(device, shadow_image_view, NULL);
    vmaDestroyImage(allocator, shadow_image, shadow_image_allocation);
}