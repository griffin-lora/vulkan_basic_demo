#version 450

layout(push_constant, std430) uniform vertex_push_constants_t {
    mat4 model_view_projection;
};

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 tex_coord;

layout(location = 0) out vec3 frag_position;
layout(location = 1) out vec3 frag_normal;
layout(location = 2) out vec2 frag_tex_coord;

void main() {
    vec4 out_position = model_view_projection * vec4(position, 1.0);
    gl_Position = out_position;
    frag_position = out_position.xyz;
    frag_normal = normal;
    frag_tex_coord = tex_coord;
}