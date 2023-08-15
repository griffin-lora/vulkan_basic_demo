#version 450

layout(push_constant, std430) uniform push_constants_t {
    mat4 model_view_projection;
    vec3 camera_position;
};

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 tangent;
layout(location = 3) in vec2 tex_coord;

layout(location = 0) out vec2 frag_tex_coord;

// All in normal texture space
layout(location = 1) out vec3 frag_vertex_to_camera_direction;
layout(location = 2) out vec3 frag_light_direction;
layout(location = 3) out vec3 frag_position_to_light_direction;

vec3 light_direction = normalize(vec3(-2.0, -1.0, 0.0));
vec3 position_to_light_direction = -light_direction;

void main() {
	gl_Position = model_view_projection * vec4(position, 1.0);

	vec3 unit_normal = normalize(normal);
	vec3 unit_tangent = normalize(tangent.xyz);

	// Model is identity so this is not needed: normalize(inverse(mat3(model)))
	vec3 bitangent = cross(unit_normal, unit_tangent) * tangent.w;
	mat3 normal_texture_matrix = transpose(mat3(unit_tangent, bitangent, unit_normal)); // Transpose is inverse here since it is orthogonal

	frag_tex_coord = tex_coord;
	frag_vertex_to_camera_direction = normal_texture_matrix * normalize(camera_position - position) /* model is identity so we can use position as it is the world position as well */;
	frag_light_direction = normal_texture_matrix * light_direction;
	frag_position_to_light_direction = normal_texture_matrix * position_to_light_direction;
}