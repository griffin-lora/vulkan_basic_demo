#version 450

layout(push_constant, std430) uniform push_constants_t {
    mat4 model_view_projection;
    mat4 view;
};

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 tangent;
layout(location = 3) in vec2 tex_coord;

layout(location = 0) out vec3 frag_world_position;
layout(location = 1) out vec3 frag_view_normal;
layout(location = 2) out vec3 frag_vertex_to_camera_view_position;
layout(location = 3) out vec3 frag_light_view_direction;
layout(location = 4) out vec3 frag_light_tangent_direction;
layout(location = 5) out vec3 frag_vertex_to_camera_tangent_position;
layout(location = 6) out vec2 frag_tex_coord;

void main() {
    vec3 light_world_position = vec3(12.0);
	gl_Position = model_view_projection * vec4(position, 1.0);
	
	vec3 vertex_view_position = (view * vec4(position, 1.0)).xyz;
	vec3 light_view_position = (view * vec4(light_world_position, 1.0)).xyz;
    
    mat3 view_nontranslate = mat3(view);
	vec3 bitangent = cross(normal, tangent.xyz) * tangent.w;
	mat3 tangent_matrix = transpose(view_nontranslate * mat3(tangent.xyz, bitangent, normal));

	frag_world_position = vec4(position, 1.0).xyz;
	frag_view_normal = (view * vec4(normal, 0.0)).xyz;
	frag_vertex_to_camera_view_position = vec3(0.0) - vertex_view_position;
	frag_light_view_direction = light_view_position + frag_vertex_to_camera_view_position;
	frag_light_tangent_direction = tangent_matrix * frag_light_view_direction;
	frag_vertex_to_camera_tangent_position = tangent_matrix * frag_vertex_to_camera_view_position;
	frag_tex_coord = tex_coord;
}

