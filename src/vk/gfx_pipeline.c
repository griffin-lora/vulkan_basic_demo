#include "gfx_pipeline.h"
#include "core.h"
#include "gfx_core.h"
#include "ds.h"
#include "util.h"
#include "mesh.h"
#include <malloc.h>
#include <string.h>

const char* init_vulkan_graphics_pipeline(const VkPhysicalDeviceProperties* physical_device_properties) {
    //

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

        if (vkCreateRenderPass(device, &info, NULL, &render_pass) != VK_SUCCESS) {
            return "Failed to create render pass\n";
        }
    }

    //

    const char* image_paths[] = {
        "image/test_color.jpg",
        "image/test_normal.jpg"
    };

    image_extent_t image_extents[NUM_TEXTURE_IMAGES];
    uint32_t num_mip_levels_array[NUM_TEXTURE_IMAGES];
    VkBuffer image_staging_buffers[NUM_TEXTURE_IMAGES];
    VmaAllocation image_staging_allocations[NUM_TEXTURE_IMAGES];

    if (begin_images(NUM_TEXTURE_IMAGES, image_paths, image_extents, num_mip_levels_array, image_staging_buffers, image_staging_allocations, texture_images, texture_image_allocations) != result_success) {
        return "Failed to begin creating images\n";
    }

    size_t num_vertices;
    vertex_array_t vertex_arrays[NUM_VERTEX_ARRAYS];
    uint16_t* indices;
    {
        mesh_t mesh;
        if (load_gltf_mesh("mesh/test.gltf", &mesh) != result_success) {
            return "Failed to load mesh\n";
        }

        num_vertices = mesh.num_vertices;
        num_indices = mesh.num_indices;
        memcpy(vertex_arrays, mesh.vertex_arrays, sizeof(mesh.vertex_arrays));
        indices = mesh.indices;
    }

    VkBuffer vertex_staging_buffers[NUM_VERTEX_ARRAYS];
    VmaAllocation vertex_staging_buffer_allocations[NUM_VERTEX_ARRAYS];
    for (size_t i = 0; i < NUM_VERTEX_ARRAYS; i++) {
        vertex_array_t vertex_array = vertex_arrays[i];
        size_t num_vertex_array_bytes = num_vertices*num_vertex_bytes_array[i];

        if (create_buffer(num_vertex_array_bytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertex_buffers[i], &vertex_buffer_allocations[i]) != result_success) {
            return "Failed to create vertex buffer\n";
        }

        if (create_buffer(num_vertex_array_bytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertex_staging_buffers[i], &vertex_staging_buffer_allocations[i]) != result_success) {
            return "Failed to create vertex staging buffer\n";
        }

        if (write_to_staging_buffer(vertex_staging_buffer_allocations[i], num_vertex_array_bytes, vertex_array.data) != result_success) {
            return "Failed to write to vertex staging buffer\n";
        }

        free(vertex_array.data);
    }


    if (create_buffer(num_indices*sizeof(uint16_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &index_buffer, &index_buffer_allocation) != result_success) {
        return "Failed to create index buffer\n";
    }

    //

    VkBuffer index_staging_buffer;
    VmaAllocation index_staging_buffer_allocation;
    if (create_buffer(num_indices*sizeof(uint16_t), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &index_staging_buffer, &index_staging_buffer_allocation) != result_success) {
        return "Failed to create index staging buffer\n";
    }

    if (write_to_staging_buffer(index_staging_buffer_allocation, num_indices*sizeof(uint16_t), indices) != result_success) {
        return "Failed to write to index staging buffer\n";
    }

    free(indices);

    VkCommandBuffer command_buffer;
    {
        VkCommandBufferAllocateInfo info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandPool = command_pool, // TODO: Use separate command pool
            .commandBufferCount = 1
        };

        if (vkAllocateCommandBuffers(device, &info, &command_buffer) != VK_SUCCESS) {
            return "Failed to create transfer command buffer\n";
        }
    }

    {
        VkCommandBufferBeginInfo info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };
        if (vkBeginCommandBuffer(command_buffer, &info) != VK_SUCCESS) {
            return "Failed to write to transfer command buffer\n";
        }
    }

    {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(physical_device, VK_FORMAT_R8G8B8_SRGB, &properties);

        if (!(properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            return "Texture image format does not support linear blitting\n";
        }
    }

    transfer_images(command_buffer, NUM_TEXTURE_IMAGES, image_extents, num_mip_levels_array, image_staging_buffers, texture_images);

    for (size_t i = 0; i < NUM_VERTEX_ARRAYS; i++) {
        transfer_from_staging_buffer_to_buffer(command_buffer, num_vertices*num_vertex_bytes_array[i], vertex_staging_buffers[i], vertex_buffers[i]);
    }

    transfer_from_staging_buffer_to_buffer(command_buffer, num_indices*sizeof(uint16_t), index_staging_buffer, index_buffer);

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

    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);

    end_images(NUM_TEXTURE_IMAGES, image_staging_buffers, image_staging_allocations);
    for (size_t i = 0; i < NUM_VERTEX_ARRAYS; i++) {
        vmaDestroyBuffer(allocator, vertex_staging_buffers[i], vertex_staging_buffer_allocations[i]);
    }
    vmaDestroyBuffer(allocator, index_staging_buffer, index_staging_buffer_allocation);

    //

    if (create_image_views(NUM_TEXTURE_IMAGES, num_mip_levels_array, texture_images, texture_image_views) != result_success) {
        return "Failed to create texture image view\n";
    }

    {
        VkSamplerCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .minFilter = VK_FILTER_LINEAR, // undersample
            .magFilter = VK_FILTER_LINEAR, // oversample
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .anisotropyEnable = VK_TRUE,
            .maxAnisotropy = physical_device_properties->limits.maxSamplerAnisotropy,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE, // interesting
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .minLod = 0.0f,
            .maxLod = num_mip_levels_array[0],
            .mipLodBias = 0.0f
        };

        if (vkCreateSampler(device, &info, NULL, &world_texture_image_sampler) != VK_SUCCESS) {
            return "Failed to create tetxure image sampler\n";
        }
    }

    // for (size_t i = 0; i < NUM_FRAMES_IN_FLIGHT; i++) {
    //     if (create_mapped_buffer(sizeof(clip_space), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &clip_space_uniform_buffers[i], &clip_space_uniform_buffers_allocation[i], &mapped_clip_spaces[i]) != result_success) {
    //         return "Failed to create uniform buffer\n";
    //     }
    // }

    //

    VkShaderModule vertex_shader_module;
    if (create_shader_module("shader/vertex.spv", &vertex_shader_module) != result_success) {
        return "Failed to create vertex shader module\n";
    }

    VkShaderModule fragment_shader_module;
    if (create_shader_module("shader/fragment.spv", &fragment_shader_module) != result_success) {
        return "Failed to create vertex shader module\n";
    }

    {
        descriptor_binding_t bindings[] = {
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
            }
        };

        vertex_attribute_t attributes[] = {
            {
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(vertex_t, position)
            },
            {
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(vertex_t, normal)
            },
            {
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                .offset = offsetof(vertex_t, tangent)
            },
            {
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(vertex_t, tex_coord)
            }
        };

        const char* msg = create_graphics_pipeline(
            vertex_shader_module, fragment_shader_module,
            NUM_ELEMS(bindings), bindings, infos,
            sizeof(vertex_t), NUM_ELEMS(attributes), attributes,
            sizeof(push_constants),
            render_pass,
            &descriptor_set_layout, &descriptor_pool, &descriptor_set, &pipeline_layout, &pipeline
        );
        if (msg != NULL) {
            return msg;
        }
    }
    
    vkDestroyShaderModule(device, vertex_shader_module, NULL);
    vkDestroyShaderModule(device, fragment_shader_module, NULL);

    return NULL;
}