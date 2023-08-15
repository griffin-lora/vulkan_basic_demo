#include "asset.h"
#include "util.h"
#include "mesh.h"
#include "core.h"
#include "gfx_core.h"
#include <malloc.h>
#include <string.h>
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/struct/cam.h>
#include <cglm/struct/vec2.h>
#include <cglm/struct/vec3.h>
#include <cglm/struct/mat3.h>
#include <cglm/struct/affine.h>

const char* init_vulkan_assets(const VkPhysicalDeviceProperties* physical_device_properties) {
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
    uint16_t* indices;
    vertex_array_t vertex_arrays[NUM_VERTEX_ARRAYS];
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
    if (begin_vertex_arrays(num_vertices, NUM_VERTEX_ARRAYS, &vertex_arrays[0].data, num_vertex_bytes_array, vertex_staging_buffers, vertex_staging_buffer_allocations, vertex_buffers, vertex_buffer_allocations) != result_success) {
        return "Failed to begin creating vertex buffers\n";
    }

    VkBuffer index_staging_buffer;
    VmaAllocation index_staging_buffer_allocation;
    if (begin_indices(sizeof(uint16_t), num_indices, indices, &index_staging_buffer, &index_staging_buffer_allocation, &index_buffer, &index_buffer_allocation) != result_success) {
        return "Failed to begin creating index buffer\n";
    }

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
    transfer_vertex_arrays(command_buffer, num_vertices, NUM_VERTEX_ARRAYS, num_vertex_bytes_array, vertex_staging_buffers, vertex_buffers);
    transfer_indices(command_buffer, sizeof(uint16_t), num_indices, index_staging_buffer, index_buffer);

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
    end_vertex_arrays(NUM_VERTEX_ARRAYS, vertex_staging_buffers, vertex_staging_buffer_allocations);
    end_indices(index_staging_buffer, index_staging_buffer_allocation);

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

        if (vkCreateSampler(device, &info, NULL, &texture_image_sampler) != VK_SUCCESS) {
            return "Failed to create tetxure image sampler\n";
        }
    }

    //
    mat4s projection = glms_perspective((GLM_PI*2.0f) / 5.0f, 1, 0.01f, 300.0f);
    mat4s view = glms_look((vec3s) {{ 5.5f, 4.0f, -6.0f }}, (vec3s) {{ -0.5f, -0.5f, 0.5f }}, (vec3s) {{ 0.0f, -1.0f, 0.0f }});

    mat4s shadow_model_view_projection = glms_mat4_mul(projection, view);

    if (create_buffer(sizeof(shadow_model_view_projection), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &shadow_model_view_projection_buffer, &shadow_model_view_projection_buffer_allocation) != result_success) {
        return "Failed to create shadow view projection buffer\n";
    }

    if (write_to_buffer(shadow_model_view_projection_buffer_allocation, sizeof(shadow_model_view_projection), &shadow_model_view_projection)) {
        return "Failed to write to shadow view projection buffer\n";
    }

    return NULL;
}

void term_vulkan_assets(void) {
    vmaDestroyBuffer(allocator, shadow_model_view_projection_buffer, shadow_model_view_projection_buffer_allocation);

    vkDestroySampler(device, texture_image_sampler, NULL);
    destroy_images(NUM_TEXTURE_IMAGES, texture_images, texture_image_allocations, texture_image_views);

    for (size_t i = 0; i < NUM_VERTEX_ARRAYS; i++) {
        vmaDestroyBuffer(allocator, vertex_buffers[i], vertex_buffer_allocations[i]);
    }
    vmaDestroyBuffer(allocator, index_buffer, index_buffer_allocation);
}