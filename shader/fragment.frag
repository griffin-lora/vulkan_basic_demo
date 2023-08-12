#version 450

layout(binding = 0) uniform sampler2D texture_image_sampler;

layout(location = 0) in vec3 frag_normal;
layout(location = 1) in vec2 frag_tex_coord;

layout(location = 0) out vec4 color;

void main() {
    vec4 albedo_color = texture(texture_image_sampler, frag_tex_coord);

    vec4 light_color = vec4(1.0);
    vec4 ambient_color = vec4(0.1);
    vec3 light_dir = normalize(vec3(1.0));

    float cos_theta = clamp(dot(frag_normal, light_dir), 0.0, 1.0);
    color = (ambient_color * albedo_color) + (albedo_color * light_color * cos_theta);
}