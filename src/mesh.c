#include "mesh.h"
#include <objpar.h>
#include <malloc.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

result_t load_obj_mesh(const char* path, mesh_t* mesh) {
    if (access(path, F_OK) != 0) {
        return result_failure;
    }

    FILE* file = fopen(path, "r");
    if (file == NULL) {
        return result_failure;
    }

    struct stat st;
    stat(path, &st);

    char* contents = memalign(64, st.st_size);
    if (fread(contents, st.st_size, 1, file) != 1) {
        return result_failure;
    }

    struct objpar_data data;
    void* buffer = memalign(64, objpar_get_size(contents, st.st_size));

    if (!objpar(contents, st.st_size, buffer, &data)) {
        return result_failure;
    }

    *mesh = (mesh_t) {
        .num_vertices = data.position_count,
        .num_indices = data.face_count * 3,
        .positions = (vec3s*)data.p_positions,
        .tex_coords = (vec2s*)data.p_texcoords,
        .normals = (vec3s*)data.p_normals,
        .indices = (uint32_t*)data.p_faces
    };
    return result_success;
}