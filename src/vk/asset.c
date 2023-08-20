#include "asset.h"
#include "util.h"
#include "mesh.h"
#include "core.h"
#include "gfx_core.h"
#include "color_pipeline.h"
#include "defaults.h"
#include <malloc.h>
#include <string.h>
#include <stb_image.h>
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

    void* pixel_arrays[NUM_TEXTURE_IMAGES][NUM_TEXTURE_LAYERS];

    image_create_info_t image_create_infos[NUM_TEXTURE_IMAGES] = {
        {
            .num_pixel_bytes = 4,
            .info = {
                DEFAULT_VK_SAMPLED_IMAGE,
                .format = VK_FORMAT_R8G8B8A8_SRGB,
                .arrayLayers = NUM_TEXTURE_LAYERS
            }
        },
        {
            .num_pixel_bytes = 4,
            .info = {
                DEFAULT_VK_SAMPLED_IMAGE,
                .format = VK_FORMAT_R8G8B8A8_UNORM, // USE UNORM FOR ANY NON COLOR TEXTURE, SRGB WILL FUCK UP YOUR NORMAL TEXTURE SO BAD
                .arrayLayers = NUM_TEXTURE_LAYERS
            }
        },
        {
            .num_pixel_bytes = 4,
            .info = {
                DEFAULT_VK_SAMPLED_IMAGE,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .arrayLayers = NUM_TEXTURE_LAYERS
            }
        },
    };

    int image_channels;

    for (size_t i = 0; i < NUM_TEXTURE_IMAGES; i++) {
        int width;
        int height;

        image_create_info_t* info = &image_create_infos[i];
        info->pixel_arrays = (void**)&pixel_arrays[i];
        
        for (size_t j = 0; j < info->info.arrayLayers; j++) {
            int new_width;
            int new_height;
            info->pixel_arrays[j] = stbi_load(image_paths[i][j], &new_width, &new_height, &image_channels, STBI_rgb_alpha);

            if (info->pixel_arrays[j] == NULL) {
                return "Failed to load image pixels\n";
            }

            if (j > 0 && (new_width != width || new_width != height)) {
                return "Pixels \n";
            }

            width = new_width;
            height = new_height;

            info->info.extent.width = width;
            info->info.extent.height = height;
            info->info.mipLevels = ((uint32_t)floorf(log2f(max_uint32(width, height)))) + 1;
        }
    }

    staging_t image_stagings[NUM_TEXTURE_IMAGES];

    if (begin_images(NUM_TEXTURE_IMAGES, image_create_infos, image_stagings, texture_images, texture_image_allocations) != result_success) {
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
        void* matrices_data;
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

        if (begin_buffers(mesh.num_vertices, &vertex_buffer_create_info, NUM_VERTEX_ARRAYS, &mesh.vertex_arrays[0].data, num_vertex_bytes_array, vertex_staging_arrays[i], vertex_buffer_arrays[i], vertex_buffer_allocation_arrays[i]) != result_success) {
            return "Failed to begin creating vertex buffers\n"; 
        }

        if (begin_buffers(mesh.num_indices, &index_buffer_create_info, 1, &mesh.indices_data, &num_index_bytes, &index_stagings[i], &index_buffers[i], &index_buffer_allocations[i]) != result_success) {
            return "Failed to begin creating index buffer\n";
        }

        if (begin_buffers(num_instances_array[i], &vertex_buffer_create_info, 1, &model_matrix_arrays[i].matrices_data, &num_instance_bytes, &instance_stagings[i], &instance_buffers[i], &instance_buffer_allocations[i]) != result_success) {
            return "Failed to begin creating instance buffer\n";
        }

        for (size_t i = 0; i < NUM_VERTEX_ARRAYS; i++) {
            free(mesh.vertex_arrays[i].data);
        }
        free(mesh.indices_data);
    }

    //

    vec3s light_direction = glms_vec3_normalize((vec3s) {{ -0.8f, -0.6f, 0.4f }});
    vec3s light_position = glms_vec3_scale(glms_vec3_negate(light_direction), 40.0f);

    mat4s projection = glms_ortho(-60.0f, 60.0f, -60.0f, 60.0f, 0.01f, 300.0f);

    mat4s view = glms_look(light_position, light_direction, (vec3s) {{ 0.0f, -1.0f, 0.0f }});

    mat4s shadow_view_projection = glms_mat4_mul(projection, view);
    void* shadow_view_projection_ptr = &shadow_view_projection;

    VkDeviceSize num_shadow_view_projection_bytes = sizeof(shadow_view_projection);

    staging_t shadow_view_projection_staging;
    if (begin_buffers(1, &uniform_buffer_create_info, 1, &shadow_view_projection_ptr, &num_shadow_view_projection_bytes, &shadow_view_projection_staging, &shadow_view_projection_buffer, &shadow_view_projection_buffer_allocation) != result_success) {
        return "Failed to create shadow view projection buffer\n";
    }

    //

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

    transfer_images(command_buffer, NUM_TEXTURE_IMAGES, image_create_infos, image_stagings, texture_images);

    for (size_t i = 0; i < NUM_MODELS; i++) {
        transfer_buffers(command_buffer, num_vertices_array[i], NUM_VERTEX_ARRAYS, num_vertex_bytes_array, vertex_staging_arrays[i], vertex_buffer_arrays[i]);
        transfer_buffers(command_buffer, num_indices_array[i], 1, &num_index_bytes, &index_stagings[i], &index_buffers[i]);
        transfer_buffers(command_buffer, num_instances_array[i], 1, &num_instance_bytes, &instance_stagings[i], &instance_buffers[i]);
    }
    transfer_buffers(command_buffer, 1, 1, &num_shadow_view_projection_bytes, &shadow_view_projection_staging, &shadow_view_projection_buffer);

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

    end_images(NUM_TEXTURE_IMAGES, image_stagings);

    for (size_t i = 0; i < NUM_MODELS; i++) {
        end_buffers(NUM_VERTEX_ARRAYS, vertex_staging_arrays[i]);
        end_buffers(1, &index_stagings[i]);
        end_buffers(1, &instance_stagings[i]);
    }
    end_buffers(1, &shadow_view_projection_staging);

    //

    if (create_image_views(NUM_TEXTURE_IMAGES, image_create_infos, texture_images, texture_image_views) != result_success) {
        return "Failed to create texture image view\n";
    }

    {
        VkSamplerCreateInfo info = {
            DEFAULT_VK_SAMPLER,
            .maxLod = image_create_infos[0].info.mipLevels
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