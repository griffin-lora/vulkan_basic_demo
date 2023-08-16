#version 450

layout(push_constant, std430) uniform push_constants_t {
    mat4 model_view_projection;
    vec3 camera_position;
};

layout(binding = 2) uniform sampler2D color_sampler;
layout(binding = 3) uniform sampler2D normal_sampler;
layout(binding = 4) uniform sampler2DShadow shadow_sampler;

layout(location = 0) in vec2 frag_tex_coord;

// All in normal texture space
layout(location = 1) in vec3 frag_vertex_to_camera_direction;
layout(location = 2) in vec3 frag_light_direction;
layout(location = 3) in vec3 frag_vertex_to_light_direction;
layout(location = 4) in vec3 frag_shadow_norm_device_coord;

layout(location = 0) out vec4 color;

vec3 light_base_color = vec3(0.9, 0.95, 1.0);
float ambient_base_scalar = 0.2;
float diffuse_base_scalar = 1.6;
float specular_base_scalar = 0.5;

float specular_intensity = 5.0;

float get_shadow_scalar() {
	if (
		abs(frag_shadow_norm_device_coord.x) > 1.0 ||
		abs(frag_shadow_norm_device_coord.y) > 1.0 ||
		abs(frag_shadow_norm_device_coord.z) > 1.0
	) { return 0.0; }
	vec2 shadow_tex_coord = (0.5 * frag_shadow_norm_device_coord.xy) + vec2(0.5);
	return texture(shadow_sampler, vec3(shadow_tex_coord.xy, frag_shadow_norm_device_coord.z));
}

void main() {
	vec3 base_color = texture(color_sampler, frag_tex_coord).rgb;
	vec3 ambient_color = ambient_base_scalar * light_base_color * base_color;

	float shadow_scalar = get_shadow_scalar();

	// All of this is done with normal texture (aka inverse normal space) space vectors
	vec3 vertex_to_camera_direction = normalize(frag_vertex_to_camera_direction);
	vec3 light_direction = normalize(frag_light_direction);
	vec3 vertex_to_light_direction = normalize(frag_vertex_to_light_direction);

	// A bit of a hack but using the trivial normal for specular highlight test so that the normals from the texture don't cause specular highlights in the shadow
	float cos_trivial_normal_to_vertex_to_light = clamp(dot(vec3(0.0, 0.0, 1.0), vertex_to_light_direction), 0.0, 1.0);

	// vec3 normal = normalize(2.0 * (texture(normal_sampler, frag_tex_coord).xyz - vec3(0.5)));
	vec3 normal = vec3(0.0, 0.0, 1.0);
	float cos_normal_to_vertex_to_light = clamp(dot(normal, vertex_to_light_direction), 0.0, 1.0);
	vec3 diffuse_color = shadow_scalar * cos_normal_to_vertex_to_light * diffuse_base_scalar * light_base_color * base_color;

	vec3 specular_color = vec3(0.0);
	if (cos_trivial_normal_to_vertex_to_light > 0.0) {
		vec3 reflection_direction = reflect(light_direction, normal);
		float specular_factor = clamp(dot(reflection_direction, vertex_to_camera_direction), 0.0, 1.0);
		specular_color = shadow_scalar * pow(specular_factor, specular_intensity) * cos_trivial_normal_to_vertex_to_light * specular_base_scalar * light_base_color;
	}

    color = vec4(ambient_color + diffuse_color + specular_color, 1.0);
}