#include "asset.h"
#include "util.h"
#include "mesh.h"
#include "core.h"
#include "gfx_core.h"
#include "color_pipeline.h"
#include "defaults.h"
#include <malloc.h>
#include <string.h>
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/struct/cam.h>
#include <cglm/struct/vec2.h>
#include <cglm/struct/vec3.h>
#include <cglm/struct/mat3.h>
#include <cglm/struct/affine.h>

const char* init_vulkan_assets(const VkPhysicalDeviceProperties* physical_device_properties) {
    const char* image_paths[][NUM_TEXTURE_LAYERS] = {
        {
            "image/cube_color.tga",
            "image/plane_color.jpg"
        },
        {
            "image/cube_normal.tga",
            "image/plane_normal.png"
        },
        {
            "image/cube_specular.tga",
            "image/plane_specular.png"
        }
    };
    
    VkFormat image_formats[] = {
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_R8G8B8A8_UNORM, // USE UNORM FOR ANY NON COLOR TEXTURE, SRGB WILL FUCK UP YOUR NORMAL TEXTURE SO BAD
        VK_FORMAT_R8G8B8A8_UNORM
    };

    image_extent_t image_extents[NUM_TEXTURE_IMAGES];
    uint32_t num_mip_levels_array[NUM_TEXTURE_IMAGES];
    VkBuffer image_staging_buffers[NUM_TEXTURE_IMAGES];
    VmaAllocation image_staging_allocations[NUM_TEXTURE_IMAGES];

    if (begin_images(NUM_TEXTURE_IMAGES, NUM_TEXTURE_LAYERS, image_paths, image_formats, image_extents, num_mip_levels_array, image_staging_buffers, image_staging_allocations, texture_images, texture_image_allocations) != result_success) {
        return "Failed to begin creating images\n";
    }

    const char* mesh_paths[] = {
        "mesh/cube.gltf",
        "mesh/plane.gltf"
    };

    mat4s cube_model_matrices[81];
    {
        size_t i = 0;
        for (float x = -4.0f; x <= 4.0f; x++) {
            for (float y = -4.0f; y <= 4.0f; y++, i++) {
                cube_model_matrices[i] = glms_translate(glms_mat4_identity(), (vec3s) {{ x * 8.0f, 1.0f, y * 8.0f }});
            }
        }
    }

    mat4s plane_model_matrix = glms_scale(glms_mat4_identity(), (vec3s) {{ 40.0f, 40.0f, 40.0f }});
    // mat4s plane_model_matrix = glms_mat4_identity();

    num_instances_array[0] = NUM_ELEMS(cube_model_matrices);
    num_instances_array[1] = 1;
    union {
        mat4s* matrices;
        void* matrices_raw;
    } model_matrix_arrays[] = {
        { cube_model_matrices },
        { &plane_model_matrix }
    };

    uint32_t num_vertices_array[NUM_MODELS];

    staging_t vertex_staging_arrays[NUM_MODELS][NUM_VERTEX_ARRAYS];
    staging_t index_stagings[NUM_MODELS];
    staging_t instance_stagings[NUM_MODELS];

    VkDeviceSize num_index_bytes = sizeof(uint16_t);
    VkDeviceSize num_instance_bytes = sizeof(mat4s);

    for (size_t i = 0; i < NUM_MODELS; i++) {
        mesh_t mesh;
        if (load_gltf_mesh(mesh_paths[i], &mesh) != result_success) {
            return "Failed to load mesh\n";
        }

        num_vertices_array[i] = mesh.num_vertices;
        num_indices_array[i] = mesh.num_indices;

        if (begin_buffers(mesh.num_vertices, &default_vertex_buffer_create_info, NUM_VERTEX_ARRAYS, &mesh.vertex_arrays[0].data, num_vertex_bytes_array, vertex_staging_arrays[i], vertex_buffer_arrays[i], vertex_buffer_allocation_arrays[i]) != result_success) {
            return "Failed to begin creating vertex buffers\n"; 
        }

        if (begin_buffers(mesh.num_indices, &default_index_buffer_create_info, 1, &mesh.indices_raw, &num_index_bytes, &index_stagings[i], &index_buffers[i], &index_buffer_allocations[i]) != result_success) {
            return "Failed to begin creating index buffer\n";
        }

        if (begin_buffers(num_instances_array[i], &default_vertex_buffer_create_info, 1, &model_matrix_arrays[i].matrices_raw, &num_instance_bytes, &instance_stagings[i], &instance_buffers[i], &instance_buffer_allocations[i]) != result_success) {
            return "Failed to begin creating instance buffer\n";
        }

        for (size_t i = 0; i < NUM_VERTEX_ARRAYS; i++) {
            free(mesh.vertex_arrays[i].data);
        }
        free(mesh.indices_raw);
    }

    VkCommandBuffer command_buffer;
    {
        VkCommandBufferAllocateInfo info = {
            DEFAULT_VK_COMMAND_BUFFER,
            .commandPool = command_pool // TODO: Use separate command pool
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

    transfer_images(command_buffer, NUM_TEXTURE_IMAGES, NUM_TEXTURE_LAYERS, image_extents, num_mip_levels_array, image_staging_buffers, texture_images);

    for (size_t i = 0; i < NUM_MODELS; i++) {
        transfer_buffers(command_buffer, num_vertices_array[i], NUM_VERTEX_ARRAYS, num_vertex_bytes_array, vertex_staging_arrays[i], vertex_buffer_arrays[i]);
        transfer_buffers(command_buffer, num_indices_array[i], 1, &num_index_bytes, &index_stagings[i], &index_buffers[i]);
        transfer_buffers(command_buffer, num_instances_array[i], 1, &num_instance_bytes, &instance_stagings[i], &instance_buffers[i]);
    }

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

    for (size_t i = 0; i < NUM_MODELS; i++) {
        end_buffers(NUM_VERTEX_ARRAYS, vertex_staging_arrays[i]);
        end_buffers(1, &index_stagings[i]);
        end_buffers(1, &instance_stagings[i]);
    }

    //

    if (create_image_views(NUM_TEXTURE_IMAGES, NUM_TEXTURE_LAYERS, image_formats, num_mip_levels_array, texture_images, texture_image_views) != result_success) {
        return "Failed to create texture image view\n";
    }

    {
        VkSamplerCreateInfo info = {
            DEFAULT_VK_SAMPLER,
            .maxLod = num_mip_levels_array[0]
        };

        if (vkCreateSampler(device, &info, NULL, &texture_image_sampler) != VK_SUCCESS) {
            return "Failed to create tetxure image sampler\n";
        }
    }

    {
        VkSamplerCreateInfo info = {
            DEFAULT_VK_SAMPLER,
            .compareEnable = VK_TRUE,
            .compareOp = VK_COMPARE_OP_LESS
        };

        if (vkCreateSampler(device, &info, NULL, &shadow_texture_image_sampler) != VK_SUCCESS) {
            return "Failed to create tetxure image sampler\n";
        }
    }

    //
    vec3s light_direction = glms_vec3_normalize((vec3s) {{ -0.8f, -0.6f, 0.4f }});
    vec3s light_position = glms_vec3_scale(glms_vec3_negate(light_direction), 40.0f);

    mat4s projection = glms_ortho(-60.0f, 60.0f, -60.0f, 60.0f, 0.01f, 300.0f);

    mat4s view = glms_look(light_position, light_direction, (vec3s) {{ 0.0f, -1.0f, 0.0f }});

    mat4s shadow_view_projection = glms_mat4_mul(projection, view);

    {
        VkBufferCreateInfo buffer_info = {
            DEFAULT_VK_BUFFER,
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .size = sizeof(shadow_view_projection)
        };

        VmaAllocationCreateInfo allocation_info = {
            DEFAULT_VMA_ALLOCATION,
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };

        if (vmaCreateBuffer(allocator, &buffer_info, &allocation_info, &shadow_view_projection_buffer, &shadow_view_projection_buffer_allocation, NULL) != VK_SUCCESS) {
            return "Failed to create shadow view projection buffer\n";
        }
    }

    if (write_to_buffer(shadow_view_projection_buffer_allocation, sizeof(shadow_view_projection), &shadow_view_projection)) {
        return "Failed to write to shadow view projection buffer\n";
    }

    return NULL;
}

void term_vulkan_assets(void) {
    vmaDestroyBuffer(allocator, shadow_view_projection_buffer, shadow_view_projection_buffer_allocation);

    vkDestroySampler(device, texture_image_sampler, NULL);
    vkDestroySampler(device, shadow_texture_image_sampler, NULL);
    destroy_images(NUM_TEXTURE_IMAGES, texture_images, texture_image_allocations, texture_image_views);

    for (size_t i = 0; i < NUM_MODELS; i++) {
        for (size_t j = 0; j < NUM_VERTEX_ARRAYS; j++) {
            vmaDestroyBuffer(allocator, vertex_buffer_arrays[i][j], vertex_buffer_allocation_arrays[i][j]);
        }
        vmaDestroyBuffer(allocator, index_buffers[i], index_buffer_allocations[i]);
        vmaDestroyBuffer(allocator, instance_buffers[i], instance_buffer_allocations[i]);
    }
}