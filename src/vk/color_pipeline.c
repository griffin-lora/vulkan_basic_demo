#include "color_pipeline.h"
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

VkRenderPass color_pipeline_render_pass;
static VkDescriptorSetLayout descriptor_set_layout;
static VkDescriptorPool descriptor_pool;
static VkDescriptorSet descriptor_set;
static VkPipelineLayout pipeline_layout;
static VkPipeline pipeline;

static VkCommandBuffer color_command_buffers[NUM_FRAMES_IN_FLIGHT];

static VkImage color_image;
static VmaAllocation color_image_allocation;
VkImageView color_image_view;

static VkImage depth_image;
static VmaAllocation depth_image_allocation;
VkImageView depth_image_view;

color_pipeline_push_constants_t color_pipeline_push_constants;

result_t init_color_pipeline_swapchain_dependents(void) {
    if (create_image(swap_image_extent.width, swap_image_extent.height, 1, 1, surface_format.format, render_multisample_flags, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &color_image, &color_image_allocation) != result_success) {
        return result_failure;
    }
    if (create_image_view(color_image, 1, 1, surface_format.format, VK_IMAGE_ASPECT_COLOR_BIT, &color_image_view) != result_success) {
        return result_failure;
    }

    if (create_image(swap_image_extent.width, swap_image_extent.height, 1, 1, depth_image_format, render_multisample_flags, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depth_image, &depth_image_allocation) != result_success) {
        return result_failure;
    }
    if (create_image_view(depth_image, 1, 1, depth_image_format, depth_image_format == VK_FORMAT_D32_SFLOAT_S8_UINT || depth_image_format == VK_FORMAT_D24_UNORM_S8_UINT ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_DEPTH_BIT, &depth_image_view) != result_success) {
        return result_failure;
    }
    return result_success;
}

void term_color_pipeline_swapchain_dependents(void) {
    vkDestroyImageView(device, color_image_view, NULL);
    vmaDestroyImage(allocator, color_image, color_image_allocation);
    vkDestroyImageView(device, depth_image_view, NULL);
    vmaDestroyImage(allocator, depth_image, depth_image_allocation);
}

const char* init_color_pipeline(void) {
    if (init_color_pipeline_swapchain_dependents() != result_success) {
        return "Failed to create color pipeline images\n";
    }

    {
        VkCommandBufferAllocateInfo info = {
            DEFAULT_VK_COMMAND_BUFFER,
            .commandPool = command_pool,
            .commandBufferCount = NUM_FRAMES_IN_FLIGHT
        };

        if (vkAllocateCommandBuffers(device, &info, color_command_buffers) != VK_SUCCESS) {
            return "Failed to allocate command buffers\n";
        }
    }

    VkAttachmentReference color_attachment_reference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference depth_attachment_reference = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference resolve_attachment_reference = {
        .attachment = 2,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_reference,
        .pResolveAttachments = &resolve_attachment_reference,
        .pDepthStencilAttachment = &depth_attachment_reference
    };

    VkSubpassDependency subpass_dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    };

    VkAttachmentDescription attachments[] = {
        {
            DEFAULT_VK_ATTACHMENT,
            .format = surface_format.format,
            .samples = render_multisample_flags,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        },
        {
            DEFAULT_VK_ATTACHMENT,
            .format = depth_image_format,
            .samples = render_multisample_flags,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        },
        {
            DEFAULT_VK_ATTACHMENT,
            .format = surface_format.format,
            .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        }
    };

    {
        VkRenderPassCreateInfo info = {
            DEFAULT_VK_RENDER_PASS,
            .attachmentCount = NUM_ELEMS(attachments),
            .pAttachments = attachments,
            .pSubpasses = &subpass,
            .dependencyCount = 1,
            .pDependencies = &subpass_dependency
        };

        if (vkCreateRenderPass(device, &info, NULL, &color_pipeline_render_pass) != VK_SUCCESS) {
            return "Failed to create render pass\n";
        }
    }

    VkDescriptorSetLayoutBinding descriptor_bindings[] = {
        {
            DEFAULT_VK_DESCRIPTOR_BINDING,
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
        },
        {
            DEFAULT_VK_DESCRIPTOR_BINDING,
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        },
        {
            DEFAULT_VK_DESCRIPTOR_BINDING,
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        },
        {
            DEFAULT_VK_DESCRIPTOR_BINDING,
            .binding = 3,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        },
        {
            DEFAULT_VK_DESCRIPTOR_BINDING,
            .binding = 4,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
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
        },
        {
            .type = descriptor_info_type_image,
            .image = {
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .imageView = texture_image_views[0],
                .sampler = texture_image_sampler
            }
        },
        {
            .type = descriptor_info_type_image,
            .image = {
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .imageView = texture_image_views[1],
                .sampler = texture_image_sampler
            }
        },
        {
            .type = descriptor_info_type_image,
            .image = {
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .imageView = texture_image_views[2],
                .sampler = texture_image_sampler
            }
        },
        {
            .type = descriptor_info_type_image,
            .image = {
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .imageView = shadow_image_view,
                .sampler = shadow_texture_image_sampler
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
        VkPushConstantRange range = {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .size = sizeof(color_pipeline_push_constants)
        };

        VkPipelineLayoutCreateInfo info = {
            DEFAULT_VK_PIPELINE_LAYOUT,
            .pSetLayouts = &descriptor_set_layout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &range
        };

        if (vkCreatePipelineLayout(device, &info, NULL, &pipeline_layout) != VK_SUCCESS) {
            return "Failed to create pipeline layout\n";
        }
    }

    //

    VkShaderModule vertex_shader_module;
    if (create_shader_module("shader/color_pipeline_vertex.spv", &vertex_shader_module) != result_success) {
        return "Failed to create vertex shader module\n";
    }

    VkShaderModule fragment_shader_module;
    if (create_shader_module("shader/color_pipeline_fragment.spv", &fragment_shader_module) != result_success) {
        return "Failed to create fragment shader module\n";
    }

    VkPipelineShaderStageCreateInfo shader_infos[] = {
        {
            DEFAULT_VK_SHADER_STAGE,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertex_shader_module
        },
        {
            DEFAULT_VK_SHADER_STAGE,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragment_shader_module
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
        },
        {
            .binding = 2,
            .stride = num_vertex_bytes_array[COLOR_PIPELINE_VERTEX_ARRAY_INDEX],
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
        },
        {
            .binding = 2,
            .location = 5,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(color_pipeline_vertex_t, normal)
        },
        {
            .binding = 2,
            .location = 6,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = offsetof(color_pipeline_vertex_t, tangent)
        },
        {
            .binding = 2,
            .location = 7,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(color_pipeline_vertex_t, tex_coord)
        }
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = NUM_ELEMS(vertex_bindings),
        .pVertexBindingDescriptions = vertex_bindings,
        .vertexAttributeDescriptionCount = NUM_ELEMS(vertex_attributes),
        .pVertexAttributeDescriptions = vertex_attributes
    };

    VkPipelineRasterizationStateCreateInfo rasterization_info = { DEFAULT_VK_RASTERIZATION };

    VkPipelineMultisampleStateCreateInfo multisample_info = {
        DEFAULT_VK_MULTISAMPLE,
        .rasterizationSamples = render_multisample_flags
    };
    
    VkGraphicsPipelineCreateInfo info = {
        DEFAULT_VK_GRAPHICS_PIPELINE,
        .stageCount = NUM_ELEMS(shader_infos),
        .pStages = shader_infos,
        .pVertexInputState = &vertex_input_info,
        .pRasterizationState = &rasterization_info,
        .pMultisampleState = &multisample_info,
        .layout = pipeline_layout,
        .renderPass = color_pipeline_render_pass
    };

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, NULL, &pipeline) != VK_SUCCESS) {
        return "Failed to create graphics pipeline\n";
    }

    vkDestroyShaderModule(device, vertex_shader_module, NULL);
    vkDestroyShaderModule(device, fragment_shader_module, NULL);

    return NULL;
}

const char* draw_color_pipeline(size_t frame_index, size_t image_index, VkCommandBuffer* out_command_buffer) {
    VkCommandBuffer command_buffer = color_command_buffers[frame_index];
    *out_command_buffer = command_buffer;

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
        { .color = { .float32 = { 0.62f, 0.78f, 1.0f, 1.0f } } },
        { .depthStencil = { .depth = 1.0f, .stencil = 0 } },
    };

    begin_pipeline(
        command_buffer,
        swapchain_framebuffers[image_index], swap_image_extent,
        NUM_ELEMS(clear_values), clear_values,
        color_pipeline_render_pass, descriptor_set, pipeline_layout, pipeline
    );

    for (size_t i = 0; i < NUM_MODELS; i++) {
        VkBuffer pass_vertex_buffers[] = {
            instance_buffers[i],
            vertex_buffer_arrays[i][GENERAL_PIPELINE_VERTEX_ARRAY_INDEX],
            vertex_buffer_arrays[i][COLOR_PIPELINE_VERTEX_ARRAY_INDEX]
        };

        color_pipeline_push_constants.layer_index = i;

        vkCmdPushConstants(command_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(color_pipeline_push_constants), &color_pipeline_push_constants);

        bind_vertex_buffers(command_buffer, NUM_ELEMS(pass_vertex_buffers), pass_vertex_buffers);
        vkCmdBindIndexBuffer(command_buffer, index_buffers[i], 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(command_buffer, num_indices_array[i], num_instances_array[i], 0, 0, 0);
    }

    end_pipeline(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        return "Failed to end command buffer\n";
    }

    return NULL;
}

void term_color_pipeline(void) {
    vkDestroyPipeline(device, pipeline, NULL);
    vkDestroyPipelineLayout(device, pipeline_layout, NULL);
    vkDestroyRenderPass(device, color_pipeline_render_pass, NULL);
    vkDestroyDescriptorPool(device, descriptor_pool, NULL);
    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, NULL);

    term_color_pipeline_swapchain_dependents();
}