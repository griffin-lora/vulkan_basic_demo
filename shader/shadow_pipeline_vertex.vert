#version 450

layout(binding = 0) uniform shadow_uniform_constants_t {
    mat4 shadow_view_projection;
};

layout(binding = 1) uniform model_uniform_constants_t {
    mat4 models[16];
};

layout(location = 0) in vec3 position;

void main() {
	gl_Position = shadow_view_projection * models[gl_InstanceIndex] * vec4(position, 1.0);
}