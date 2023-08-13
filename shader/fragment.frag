#version 450

layout(push_constant, std430) uniform fragment_push_constants_t {
    mat4 model_view_projection;
    vec3 camera_position;
};

layout(binding = 0) uniform sampler2D color_sampler;
layout(binding = 1) uniform sampler2D normal_sampler;

layout(location = 0) in vec3 frag_position;
layout(location = 1) in mat3 frag_tangent_space;
layout(location = 4) in vec2 frag_tex_coord;

layout(location = 0) out vec4 color;

void main() {
    vec4 light_color = vec4(1.0);
    vec4 ambient_color = vec4(0.1);
    vec4 specular_color = vec4(0.3);

    vec4 base_color = texture(color_sampler, frag_tex_coord);
    vec3 texture_normal = texture(normal_sampler, frag_tex_coord).xyz * 2.0 - vec3(1.0);

    // vec3 normal = texture_normal;
    vec3 normal = normalize(frag_tangent_space * texture_normal);
    vec3 light_dir = normalize(vec3(1.0, 1.0, 0.0));

    vec4 ambient_light = ambient_color * base_color;

    float cos_theta = clamp(dot(normal, light_dir), 0.0, 1.0);
    vec4 diffuse_light = base_color * light_color * cos_theta;

    vec3 view_dir = normalize(camera_position - frag_position);
    vec3 reflection_dir = reflect(-light_dir, normal);
    float cos_alpha = clamp(dot(view_dir, reflection_dir), 0.0, 1.0);
    vec4 specular_light = specular_color * light_color * pow(cos_alpha, 8.0);

    color = ambient_light + specular_light + diffuse_light;
}