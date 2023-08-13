#include "gfx_pipeline.h"
#include "core.h"
#include "gfx_core.h"
#include "ds.h"
#include "util.h"
#include "mesh.h"
#include <stb_image.h>
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

    int image_width;
    int image_height;

    VkBuffer world_texture_image_staging_buffers[NUM_WORLD_TEXTURE_IMAGES];
    VmaAllocation world_texture_image_staging_buffer_allocations[NUM_WORLD_TEXTURE_IMAGES];

    int image_channels;
    stbi_uc* pixel_arrays[] = {
        stbi_load("image/test_color.jpg", &image_width, &image_height, &image_channels, STBI_rgb_alpha),
        stbi_load("image/test_normal.jpg", &image_width, &image_height, &image_channels, STBI_rgb_alpha)
    };
    
    uint32_t num_mip_levels = ((uint32_t)floorf(log2f(max_uint32(image_width, image_height)))) + 1;

    for (size_t i = 0; i < NUM_WORLD_TEXTURE_IMAGES; i++) {
        if (pixel_arrays[i] == NULL) {
            return "Failed to load texture image\n";
        }

        if (create_buffer(image_width*image_height*4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &world_texture_image_staging_buffers[i], &world_texture_image_staging_buffer_allocations[i]) != result_success) {
            return "Failed to create image staging buffer\n";
        }

        if (write_to_staging_buffer(world_texture_image_staging_buffer_allocations[i], image_width*image_height*4, pixel_arrays[i]) != result_success) {
            return "Failed to write to image staging buffer\n";
        }

        stbi_image_free(pixel_arrays[i]);

        if (create_image(image_width, image_height, num_mip_levels, VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT |VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &world_texture_images[i], &world_texture_image_allocations[i]) != result_success) {
            return "Failed to create texture image\n";
        }
    }

    size_t num_vertices;
    vertex_t* vertices;
    uint16_t* indices;
    {
        mesh_t mesh;
        if (load_gltf_mesh("mesh/test.gltf", &mesh) != result_success) {
            return "Failed to load mesh\n";
        }

        num_vertices = mesh.num_vertices;
        num_indices = mesh.num_indices;
        vertices = mesh.vertices;
        indices = mesh.indices;
    }

    if (create_buffer(num_vertices*sizeof(vertex_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertex_buffer, &vertex_buffer_allocation) != result_success) {
        return "Failed to create vertex buffer\n";
    }

    if (create_buffer(num_indices*sizeof(uint16_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &index_buffer, &index_buffer_allocation) != result_success) {
        return "Failed to create index buffer\n";
    }

    VkBuffer vertex_staging_buffer;
    VmaAllocation vertex_staging_buffer_allocation;
    if (create_buffer(num_vertices*sizeof(vertex_t), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertex_staging_buffer, &vertex_staging_buffer_allocation) != result_success) {
        return "Failed to create vertex staging buffer\n";
    }

    if (write_to_staging_buffer(vertex_staging_buffer_allocation, num_vertices*sizeof(vertex_t), vertices) != result_success) {
        return "Failed to write to vertex staging buffer\n";
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

    free(vertices);
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

    for (size_t i = 0; i < NUM_WORLD_TEXTURE_IMAGES; i++) {
        transition_image_layout(command_buffer, world_texture_images[i], num_mip_levels, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        transfer_from_staging_buffer_to_image(command_buffer, image_width, image_height, world_texture_image_staging_buffers[i], world_texture_images[i]);
    }

    {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(physical_device, VK_FORMAT_R8G8B8_SRGB, &properties);

        if (!(properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            return "Texture image format does not support linear blitting\n";
        }
    }
    {
        uint32_t mip_width = image_width;
        uint32_t mip_height = image_height;

        for (size_t i = 1; i < num_mip_levels; i++) {
            for (size_t j = 0; j < NUM_WORLD_TEXTURE_IMAGES; j++) {
                transition_image_layout(command_buffer, world_texture_images[j], 1, i - 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

                VkImageBlit blit = {
                    .srcOffsets[0] = { 0, 0, 0 },
                    .srcOffsets[1] = { mip_width, mip_height, 1 },
                    .srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .srcSubresource.mipLevel = i - 1,
                    .srcSubresource.baseArrayLayer = 0,
                    .srcSubresource.layerCount = 1,
                    .dstOffsets[0] = { 0, 0, 0 },
                    .dstOffsets[1] = { mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1 },
                    .dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .dstSubresource.mipLevel = i,
                    .dstSubresource.baseArrayLayer = 0,
                    .dstSubresource.layerCount = 1
                };

                vkCmdBlitImage(command_buffer, world_texture_images[j], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, world_texture_images[j], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

                transition_image_layout(command_buffer, world_texture_images[j], 1, i - 1, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            }

            if (mip_width > 1) { mip_width /= 2; }
            if (mip_height > 1) { mip_height /= 2; }
        }
    }

    for (size_t i = 0; i < NUM_WORLD_TEXTURE_IMAGES; i++) {
        transition_image_layout(command_buffer, world_texture_images[i], 1, num_mip_levels - 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }

    transfer_from_staging_buffer_to_buffer(command_buffer, num_vertices*sizeof(vertex_t), vertex_staging_buffer, vertex_buffer);
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

    for (size_t i = 0; i < NUM_WORLD_TEXTURE_IMAGES; i++) {
        vmaDestroyBuffer(allocator, world_texture_image_staging_buffers[i], world_texture_image_staging_buffer_allocations[i]);
    }
    vmaDestroyBuffer(allocator, vertex_staging_buffer, vertex_staging_buffer_allocation);
    vmaDestroyBuffer(allocator, index_staging_buffer, index_staging_buffer_allocation);

    //

    for (size_t i = 0; i < NUM_WORLD_TEXTURE_IMAGES; i++) {
        if (create_image_view(world_texture_images[i], num_mip_levels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, &world_texture_image_views[i]) != result_success) {
            return "Failed to create texture image view\n";
        }
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
            .maxLod = num_mip_levels,
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
                    .imageView = world_texture_image_views[0],
                    .sampler = world_texture_image_sampler
                }
            },
            {
                .type = descriptor_info_type_image,
                .image = {
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .imageView = world_texture_image_views[1],
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