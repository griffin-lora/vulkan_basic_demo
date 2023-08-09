#version 450

layout(binding = 1) uniform sampler2D texture_image_sampler;

layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec2 frag_tex_coord;

layout(location = 0) out vec4 color;

void main() {
    color = vec4(frag_color, 1.0) * texture(texture_image_sampler, frag_tex_coord);
}