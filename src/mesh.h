#pragma once
#include "result.h"
#include <cglm/struct/vec2.h>
#include <cglm/struct/vec3.h>

// Used on all render passes
typedef struct {
    vec3s position;
} general_pass_vertex_t;

typedef struct {
    vec3s normal;
    vec4s tangent;
    vec2s tex_coord;
} color_pass_vertex_t;

typedef union {
    void* data;
    general_pass_vertex_t* general_pass_vertices;
    color_pass_vertex_t* color_pass_vertices;
} vertex_array_t;

#define NUM_VERTEX_ARRAYS 2

extern size_t num_vertex_bytes_array[NUM_VERTEX_ARRAYS];

typedef struct {
    size_t num_vertices;
    size_t num_indices;
    vertex_array_t vertex_arrays[NUM_VERTEX_ARRAYS];
    uint16_t* indices;
} mesh_t;

result_t load_gltf_mesh(const char* path, mesh_t* mesh);