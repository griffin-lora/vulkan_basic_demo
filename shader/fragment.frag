#version 450

layout(binding = 0) uniform sampler2D color_sampler;
layout(binding = 1) uniform sampler2D normal_sampler;

layout(location = 0) in vec3 frag_normal;
layout(location = 1) in vec2 frag_tex_coord;

layout(location = 0) out vec4 color;

void main() {
    vec4 diffuse_color = texture(color_sampler, frag_tex_coord);
    vec3 texture_normal = texture(normal_sampler, frag_tex_coord).xyz * 2.0 - vec3(1.0);

    // vec3 normal = texture_normal;
    vec3 normal = frag_normal;

    vec4 light_color = vec4(1.0);
    vec4 ambient_color = vec4(0.1);
    vec3 light_dir = normalize(vec3(1.0));

    float cos_theta = clamp(dot(normal, light_dir), 0.0, 1.0);
    color = (ambient_color * diffuse_color) + (diffuse_color * light_color * cos_theta);
}