#version 450

layout(push_constant, std430) uniform push_constants_t {
    mat4 view_projection;
    vec3 camera_position;
};

layout(binding = 0) uniform shadow_uniform_constants_t {
    mat4 shadow_view_projection;
};

layout(binding = 1) uniform model_uniform_constants_t {
    mat4 model;
};

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 tangent;
layout(location = 3) in vec2 tex_coord;

layout(location = 0) out vec2 frag_tex_coord;

// All in normal texture space
layout(location = 1) out vec3 frag_vertex_to_camera_direction;
layout(location = 2) out vec3 frag_light_direction;
layout(location = 3) out vec3 frag_vertex_to_light_direction;
layout(location = 4) out vec3 frag_shadow_norm_device_coord;

vec3 light_direction = normalize(vec3(-0.879390, -0.223216, 0.420532));
vec3 vertex_to_light_direction = -light_direction;

void main() {
	gl_Position = view_projection * model * vec4(position, 1.0);

	vec3 unit_normal = normalize(normal);
	vec3 unit_tangent = normalize(tangent.xyz);

	// Model is identity so this is not needed: normalize(inverse(mat3(model)))
	vec3 bitangent = cross(unit_normal, unit_tangent) * tangent.w;
	mat3 normal_texture_matrix = transpose(mat3(unit_tangent, bitangent, unit_normal)) * transpose(mat3(model)); // Transpose is inverse here since it is orthogonal

	vec3 world_position = (model * vec4(position, 1.0)).xyz;

	frag_tex_coord = tex_coord;
	frag_vertex_to_camera_direction = normal_texture_matrix * normalize(camera_position - world_position);
	frag_light_direction = normal_texture_matrix * light_direction;
	frag_vertex_to_light_direction = normal_texture_matrix * vertex_to_light_direction;
	vec4 shadow_clip_position = shadow_view_projection * model * vec4(position, 1.0);
	frag_shadow_norm_device_coord = shadow_clip_position.xyz / shadow_clip_position.w;
}