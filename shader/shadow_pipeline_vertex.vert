#version 450

layout(binding = 0) uniform shadow_uniform_constants_t {
    mat4 shadow_view_projection;
};

layout(location = 0) in vec3 position;

void main() {
	gl_Position = shadow_view_projection * mat4(1.0) * vec4(position, 1.0);
}