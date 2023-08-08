#version 450

layout(binding = 0) uniform clip_space_matrix_t {
    mat4 clip_space_matrix;
} u;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 frag_color;

void main() {
    gl_Position = u.clip_space_matrix * vec4(position, 1.0);
    frag_color = color;
}