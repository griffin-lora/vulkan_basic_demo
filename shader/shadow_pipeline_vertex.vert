#version 450

layout(binding = 0) uniform shadow_uniform_constants_t {
    mat4 model_view_projection;
};

layout(location = 0) in vec3 position;

void main() {
	gl_Position = model_view_projection * vec4(position, 1.0);
    // gl_Position = vec4(0.3, 0.1, 0.0, 1.0);
}