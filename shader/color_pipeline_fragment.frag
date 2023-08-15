#version 450

layout(push_constant, std430) uniform push_constants_t {
    mat4 model_view_projection;
    vec3 camera_position;
};

layout(binding = 0) uniform sampler2D color_sampler;
layout(binding = 1) uniform sampler2D normal_sampler;
layout(binding = 2) uniform sampler2D shadow_sampler;

layout(location = 0) in vec2 frag_tex_coord;
layout(location = 1) in vec3 frag_normal;
layout(location = 2) in vec3 frag_vertex_to_camera_direction;

layout(location = 0) out vec4 color;

vec3 light_direction = normalize(vec3(1.0, -3.0, 1.0));
vec3 position_to_light_direction = -light_direction;

vec3 ambient_base_color = vec3(0.1);
vec3 diffuse_base_color = vec3(2.0);
vec3 specular_base_color = vec3(0.3);
float specular_intensity = 10.0;

void main() {
	vec3 normal = normalize(frag_normal);
	float cos_normal_to_position_to_light = clamp(dot(normal, position_to_light_direction), 0.0, 1.0);

	vec3 base_color = texture(color_sampler, frag_tex_coord).rgb;

	vec3 ambient_color = ambient_base_color * base_color;
	vec3 diffuse_color = vec3(base_color * diffuse_base_color * cos_normal_to_position_to_light);

	vec3 specular_color = vec3(0);
	if (cos_normal_to_position_to_light >= 0) {
		vec3 reflection_direction = reflect(light_direction, normal);
		float specular_factor = clamp(dot(reflection_direction, normalize(frag_vertex_to_camera_direction)), 0.0, 1.0);
		specular_color = specular_base_color * pow(specular_factor, specular_intensity);
	}

    color = vec4(ambient_color + diffuse_color + specular_color, 1.0);
}