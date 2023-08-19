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

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {
        DEFAULT_VK_COMMAND_BUFFER,
        .commandPool = command_pool,
        .commandBufferCount = NUM_FRAMES_IN_FLIGHT
    };

    if (vkAllocateCommandBuffers(device, &command_buffer_allocate_info, color_command_buffers) != VK_SUCCESS) {
        return "Failed to allocate command buffers\n";
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
            .format = surface_format.format,
            .samples = render_multisample_flags,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        },
        {
            .format = depth_image_format,
            .samples = render_multisample_flags,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        },
        {
            .format = surface_format.format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        }
    };

    {
        VkRenderPassCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = NUM_ELEMS(attachments),
            .pAttachments = attachments,
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 1,
            .pDependencies = &subpass_dependency
        };

        if (vkCreateRenderPass(device, &info, NULL, &color_pipeline_render_pass) != VK_SUCCESS) {
            return "Failed to create render pass\n";
        }
    }

    VkShaderModule vertex_shader_module;
    if (create_shader_module("shader/color_pipeline_vertex.spv", &vertex_shader_module) != result_success) {
        return "Failed to create vertex shader module\n";
    }

    VkShaderModule fragment_shader_module;
    if (create_shader_module("shader/color_pipeline_fragment.spv", &fragment_shader_module) != result_success) {
        return "Failed to create fragment shader module\n";
    }

    shader_t shaders[] = {
        {
            .stage_flags = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertex_shader_module
        },
        {
            .stage_flags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragment_shader_module
        }
    };

    descriptor_binding_t bindings[] = {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .stage_flags = VK_SHADER_STAGE_VERTEX_BIT
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stage_flags = VK_SHADER_STAGE_FRAGMENT_BIT
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stage_flags = VK_SHADER_STAGE_FRAGMENT_BIT
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stage_flags = VK_SHADER_STAGE_FRAGMENT_BIT
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stage_flags = VK_SHADER_STAGE_FRAGMENT_BIT
        }
    };

    descriptor_info_t infos[] = {
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
    
    vertex_binding_t vertex_bindings[] = {
        {
            .num_bytes = sizeof(mat4s),
            .input_rate = VK_VERTEX_INPUT_RATE_INSTANCE
        },
        {
            .num_bytes = num_vertex_bytes_array[GENERAL_PIPELINE_VERTEX_ARRAY_INDEX],
            .input_rate = VK_VERTEX_INPUT_RATE_VERTEX
        },
        {
            .num_bytes = num_vertex_bytes_array[COLOR_PIPELINE_VERTEX_ARRAY_INDEX],
            .input_rate = VK_VERTEX_INPUT_RATE_VERTEX
        }
    };

    vertex_attribute_t attributes[] = {
        {
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = 0*sizeof(vec4s)
        },
        {
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = 1*sizeof(vec4s)
        },
        {
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = 2*sizeof(vec4s)
        },
        {
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = 3*sizeof(vec4s)
        },
        //
        {
            .binding = 1,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(general_pipeline_vertex_t, position)
        },
        {
            .binding = 2,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(color_pipeline_vertex_t, normal)
        },
        {
            .binding = 2,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = offsetof(color_pipeline_vertex_t, tangent)
        },
        {
            .binding = 2,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(color_pipeline_vertex_t, tex_coord)
        }
    };

    const char* msg = create_graphics_pipeline(
        NUM_ELEMS(shaders), shaders,
        NUM_ELEMS(bindings), bindings, infos,
        NUM_ELEMS(vertex_bindings), vertex_bindings,
        NUM_ELEMS(attributes), attributes,
        sizeof(color_pipeline_push_constants),
        render_multisample_flags, (depth_bias_t) { .enable = false },
        color_pipeline_render_pass,
        &descriptor_set_layout, &descriptor_pool, &descriptor_set, &pipeline_layout, &pipeline
    );
    if (msg != NULL) {
        return msg;
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

        vkCmdPushConstants(command_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(color_pipeline_push_constants), &color_pipeline_push_constants);

        draw_instanced_model(
            command_buffer,
            NUM_ELEMS(pass_vertex_buffers), pass_vertex_buffers,
            num_indices_array[i], index_buffers[i],
            num_instances_array[i]
        );
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