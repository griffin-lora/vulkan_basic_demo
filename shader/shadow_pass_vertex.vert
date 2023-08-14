#version 450

layout(push_constant, std430) uniform push_constants_t {
    mat4 model_view_projection;
    mat4 view;
};

layout(location = 0) in vec3 position;

void main() {
	gl_Position = model_view_projection * vec4(position, 1.0);
}