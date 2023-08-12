#pragma once
#include "result.h"
#include <cglm/struct/vec2.h>
#include <cglm/struct/vec3.h>

typedef struct {
    vec3s position;
    vec3s normal;
    vec2s tex_coord;
} vertex_t;

typedef struct {
    size_t num_vertices;
    size_t num_indices;
    vertex_t* vertices;
    uint16_t* indices;
} mesh_t;

result_t load_glb_mesh(const char* path, mesh_t* mesh);