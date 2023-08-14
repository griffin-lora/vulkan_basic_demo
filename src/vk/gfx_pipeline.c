#include "gfx_pipeline.h"
#include "core.h"
#include "gfx_core.h"
#include "util.h"
#include "asset.h"

static const char* init_shadow_pipeline(void) {
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
        sizeof(push_constants),
        VK_SAMPLE_COUNT_1_BIT,
        shadow_pipeline.render_pass,
        NULL, NULL, NULL, &shadow_pipeline.pipeline_layout, &shadow_pipeline.pipeline
    );
    if (msg != NULL) {
        return msg;
    }

    vkDestroyShaderModule(device, vertex_shader_module, NULL);

    return NULL;
}

static const char* init_color_pipeline(void) {
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

        if (vkCreateRenderPass(device, &info, NULL, &color_pipeline.render_pass) != VK_SUCCESS) {
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
            .type = descriptor_info_type_image,
            .image = {
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .imageView = texture_image_views[0],
                .sampler = world_texture_image_sampler
            }
        },
        {
            .type = descriptor_info_type_image,
            .image = {
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .imageView = texture_image_views[1],
                .sampler = world_texture_image_sampler
            }
        },
        {
            .type = descriptor_info_type_image,
            .image = {
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .imageView = shadow_image_view,
                .sampler = world_texture_image_sampler
            }
        }
    };
    
    uint32_t num_pass_vertex_bytes_array[] = {
        num_vertex_bytes_array[GENERAL_PIPELINE_VERTEX_ARRAY_INDEX],
        num_vertex_bytes_array[COLOR_PIPELINE_VERTEX_ARRAY_INDEX],
    };

    vertex_attribute_t attributes[] = {
        {
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(general_pipeline_vertex_t, position)
        },
        {
            .binding = 1,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(color_pipeline_vertex_t, normal)
        },
        {
            .binding = 1,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = offsetof(color_pipeline_vertex_t, tangent)
        },
        {
            .binding = 1,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(color_pipeline_vertex_t, tex_coord)
        }
    };

    const char* msg = create_graphics_pipeline(
        NUM_ELEMS(shaders), shaders,
        NUM_ELEMS(bindings), bindings, infos,
        NUM_ELEMS(num_pass_vertex_bytes_array), num_pass_vertex_bytes_array,
        NUM_ELEMS(attributes), attributes,
        sizeof(push_constants),
        render_multisample_flags,
        color_pipeline.render_pass,
        &color_pipeline.descriptor_set_layout, &color_pipeline.descriptor_pool, &color_pipeline.descriptor_set, &color_pipeline.pipeline_layout, &color_pipeline.pipeline
    );
    if (msg != NULL) {
        return msg;
    }

    vkDestroyShaderModule(device, vertex_shader_module, NULL);
    vkDestroyShaderModule(device, fragment_shader_module, NULL);

    return NULL;
}

const char* init_vulkan_graphics_pipelines() {
    const char* msg = init_shadow_pipeline();
    if (msg != NULL) {
        return msg;
    }

    msg = init_color_pipeline();
    if (msg != NULL) {
        return msg;
    }

    return NULL;
}