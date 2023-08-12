#version 450

// layout(binding = 0) uniform clip_space_t {
//     mat4 elem;
// } clip_space;

layout(push_constant, std430) uniform clip_space_t {
    mat4 clip_space;
};

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 tex_coord;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_tex_coord;

void main() {
    gl_Position = clip_space * vec4(position, 1.0);
    frag_color = color;
    frag_tex_coord = tex_coord;
}