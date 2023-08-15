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
layout(location = 1) out vec3 frag_normal;
layout(location = 2) out vec3 frag_vertex_to_camera_direction;

void main() {
	gl_Position = model_view_projection * vec4(position, 1.0);
	frag_tex_coord = tex_coord;
	frag_normal = normalize(normal) /*model is identity so this is not needed: normalize(inverse(mat3(model)))*/;
	frag_vertex_to_camera_direction = normalize(camera_position - position) /* model is identity so we can use position as it is the world position as well */;
}