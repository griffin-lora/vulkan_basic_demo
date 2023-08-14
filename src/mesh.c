#include "mesh.h"
#include <stdio.h>
#include <cgltf.h>
#include <malloc.h>
#include <string.h>

size_t num_vertex_bytes_array[NUM_VERTEX_ARRAYS] = {
    [GENERAL_PASS_VERTEX_ARRAY_INDEX] = sizeof(general_pass_vertex_t),
    [COLOR_PASS_VERTEX_ARRAY_INDEX] sizeof(color_pass_vertex_t)
};

result_t load_gltf_mesh(const char* path, mesh_t* mesh) {
    cgltf_options options = {};
    cgltf_data* data = NULL;

    if (cgltf_parse_file(&options, path, &data) != cgltf_result_success) {
        return result_failure;
    }

    if (data->meshes_count != 1) {
        return result_failure;
    }

    const cgltf_mesh* mesh_data = &data->meshes[0];
    if (mesh_data->primitives_count != 1) {
        return result_failure;
    }

    const cgltf_primitive* primitive_data = &mesh_data->primitives[0];

    if (primitive_data->attributes_count != 4) {
        return result_failure;
    }
    if (primitive_data->attributes[0].type != cgltf_attribute_type_position) {
        return result_failure;
    }
    if (primitive_data->attributes[1].type != cgltf_attribute_type_normal) {
        return result_failure;
    }
    if (primitive_data->attributes[2].type != cgltf_attribute_type_tangent) {
        return result_failure;
    }
    if (primitive_data->attributes[3].type != cgltf_attribute_type_texcoord) {
        return result_failure;
    }

    if (cgltf_load_buffers(&options, data, path) != cgltf_result_success) {
        return result_failure;
    }

    cgltf_size num_vertices = primitive_data->attributes[0].data->count;

    vertex_array_t vertex_arrays[NUM_VERTEX_ARRAYS];

    for (size_t i = 0; i < NUM_VERTEX_ARRAYS; i++) {
        vertex_arrays[i].data = memalign(64, num_vertices*num_vertex_bytes_array[i]);
    }

    const vec3s* position_data = primitive_data->attributes[0].data->buffer_view->buffer->data + primitive_data->attributes[0].data->buffer_view->offset;
    const vec3s* normal_data = primitive_data->attributes[1].data->buffer_view->buffer->data + primitive_data->attributes[1].data->buffer_view->offset;
    const vec4s* tangent_data = primitive_data->attributes[2].data->buffer_view->buffer->data + primitive_data->attributes[2].data->buffer_view->offset;
    const vec2s* tex_coord_data = primitive_data->attributes[3].data->buffer_view->buffer->data + primitive_data->attributes[3].data->buffer_view->offset;

    for (size_t i = 0; i < num_vertices; i++) {
        vertex_arrays[GENERAL_PASS_VERTEX_ARRAY_INDEX].general_pass_vertices[i] = (general_pass_vertex_t) {
            .position = position_data[i]
        };
        vertex_arrays[COLOR_PASS_VERTEX_ARRAY_INDEX].color_pass_vertices[i] = (color_pass_vertex_t) {
            .normal = normal_data[i],
            .tangent = tangent_data[i],
            .tex_coord = tex_coord_data[i]
        };
    }

    cgltf_size num_indices = primitive_data->indices->count;
    uint16_t* indices = memalign(64, num_indices*sizeof(uint16_t));
    
    const uint16_t* indices_data = primitive_data->indices->buffer_view->buffer->data + primitive_data->indices->buffer_view->offset;

    memcpy(indices, indices_data, num_indices*sizeof(uint16_t));

    cgltf_free(data);

    *mesh = (mesh_t) {
        .num_vertices = num_vertices,
        .num_indices = num_indices,
        .indices = indices
    };
    memcpy(mesh->vertex_arrays, vertex_arrays, sizeof(vertex_arrays));
    return result_success;
}