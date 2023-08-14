#version 450

layout(push_constant, std430) uniform push_constants_t {
    mat4 model_view_projection;
    mat4 view;
};

layout(binding = 0) uniform sampler2D color_sampler;
layout(binding = 1) uniform sampler2D normal_sampler;
layout(binding = 2) uniform sampler2D shadow_sampler;

layout(location = 0) in vec3 frag_world_position;
layout(location = 1) in vec3 frag_view_normal;
layout(location = 2) in vec3 frag_vertex_to_camera_view_position;
layout(location = 3) in vec3 frag_light_view_direction;
layout(location = 4) in vec3 frag_light_tangent_direction;
layout(location = 5) in vec3 frag_vertex_to_camera_tangent_position;
layout(location = 6) in vec2 tex_coord;

layout(location = 0) out vec4 color;

void main(){
    vec3 light_world_position = vec3(12.0);
	vec3 light_color = vec3(1.0);
	float light_power = 2.0;
	
	vec3 base_color = texture(color_sampler, tex_coord).rgb;
	vec3 ambient_color = vec3(0.7) * base_color;
    vec3 specular_color = vec3(0.3);

	vec3 tangent_normal = normalize(texture(normal_sampler, tex_coord).rgb * 2.0 - 1.0);
	
	// float light_distance = length(light_world_position - frag_world_position);
	float light_distance = 1.0;

	vec3 light_tangent_direction = normalize(frag_light_tangent_direction);
	float cos_theta = clamp(dot(tangent_normal, light_tangent_direction), 0.0, 1.0);

	vec3 vertex_to_camera_tangent_position = normalize(frag_vertex_to_camera_tangent_position);
	vec3 reflection_tangent_direction = reflect(-light_tangent_direction, tangent_normal);
	float cos_alpha = clamp(dot(vertex_to_camera_tangent_position, reflection_tangent_direction), 0.0, 1.0);
	
	color = vec4(
		ambient_color +
		base_color * light_color * light_power * cos_theta / (light_distance*light_distance) +
		specular_color * light_color * light_power * pow(cos_alpha, 5.0) / (light_distance*light_distance),
		1.0
	);

	float shadow_depth = texture(shadow_sampler, tex_coord).r;
	// color = vec4(shadow_depth, shadow_depth, shadow_depth, 1.0);
}