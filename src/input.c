#include "input.h"
#include "vk/gfx_pipeline.h"
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/struct/cam.h>
#include <cglm/struct/mat3.h>
#include <stdbool.h>
#include <math.h>

#define M_TAU (GLM_PI * 2)

#define MOVE_SPEED 0.3f
#define ROT_SPEED 2.0f

static vec3s cam_pos = {{ 1.0f, 0.5f, -1.0f }};
static vec3s cam_vel = {{ 0.0f, 0.0f, 0.0f }};

static vec2s cam_rot = {{ -4.0f, -0.5f }};
static vec2s cam_rot_vel = {{ 0.0f, 0.0f }};

static bool in_rotation_mode = false;
static vec2s rotation_mode_cursor_position = {{ 0.0f, 0.0f }};
static vec2s rotation_mode_norm_cursor_position = {{ 0.0f, 0.0f }};

static bool is_cursor_position_out_of_bounds(vec2s cursor_position) {
    return cursor_position.x < 0 || cursor_position.x >= swap_image_extent.width || cursor_position.y < 0.0f || cursor_position.y >= swap_image_extent.height;
}

static vec2s get_norm_cursor_position(float aspect, vec2s cursor_position) {
    vec2s norm_cursor_position = glms_vec2_mul(cursor_position, (vec2s) {{ 1.0f/(float)swap_image_extent.width, 1.0f/(float)swap_image_extent.height }});
    norm_cursor_position.x *= aspect;
    return norm_cursor_position;
}

static vec2s get_desired_rotational_velocity(float aspect) {
    int mouse_button = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2);

    if (in_rotation_mode && mouse_button != GLFW_PRESS) {
        in_rotation_mode = false;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        return (vec2s) {{ 0.0f, 0.0f }};
    }
    if (!in_rotation_mode && mouse_button == GLFW_PRESS) {
        double cursor_x;
        double cursor_y;
        glfwGetCursorPos(window, &cursor_x, &cursor_y);

        const vec2s cursor_position = {{ cursor_x, cursor_y }};
        if (is_cursor_position_out_of_bounds(cursor_position)) {
            return (vec2s) {{ 0.0f, 0.0f }};
        }

        in_rotation_mode = true;
        rotation_mode_cursor_position = cursor_position;
        rotation_mode_norm_cursor_position = get_norm_cursor_position(aspect, cursor_position);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        return (vec2s) {{ 0.0f, 0.0f }};
    }

    if (!in_rotation_mode) {
        return (vec2s) {{ 0.0f, 0.0f }};
    }

    double cursor_x;
    double cursor_y;
    glfwGetCursorPos(window, &cursor_x, &cursor_y);

    const vec2s cursor_position = {{ cursor_x, cursor_y }};
    if (is_cursor_position_out_of_bounds(cursor_position)) {
        return (vec2s) {{ 0.0f, 0.0f }};
    }

    glfwSetCursorPos(window, rotation_mode_cursor_position.x, rotation_mode_cursor_position.y);
    
    return glms_vec2_scale(glms_vec2_sub(rotation_mode_norm_cursor_position, get_norm_cursor_position(aspect, cursor_position)), ROT_SPEED);
}

void handle_input(float delta) {
    float aspect = (float)swap_image_extent.width/(float)swap_image_extent.height;

    mat4s projection = glms_perspective(M_TAU / 5.0f, aspect, 0.01f, 300.0f);

    vec2s desired_rot_vel = get_desired_rotational_velocity(aspect);
    cam_rot_vel = glms_vec2_lerp(cam_rot_vel, desired_rot_vel, 0.2f);

    cam_rot = glms_vec2_add(cam_rot, cam_rot_vel);
    if (cam_rot.y <= (-M_TAU / 4) + 0.01f) {
        cam_rot.y = (-M_TAU / 4) + 0.01f;
    } else if (cam_rot.y >= (M_TAU / 4) - 0.01f) {
        cam_rot.y = (M_TAU / 4) - 0.01f;
    }

    const vec2s pitch_vector = {{ cosf(cam_rot.y), sinf(cam_rot.y) }};
    const vec2s yaw_vector = {{ cosf(cam_rot.x), sinf(cam_rot.x) }};
    vec3s cam_forward = {{ yaw_vector.x * pitch_vector.x, pitch_vector.y, yaw_vector.y * pitch_vector.x }};

    vec3s input_vec = {{ 0.0f, 0.0f, 0.0f }};
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        input_vec.x += 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        input_vec.x -= 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        input_vec.z += 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        input_vec.z -= 1.0f;
    }

    vec3s move_vec = glms_vec3_scale(input_vec, MOVE_SPEED);

    mat3s movement_mat = (mat3s) {{
        { cam_forward.x, cam_forward.y, cam_forward.z },
        { 0, 1, 0 },
        { cam_forward.z, 0, -cam_forward.x }
    }};
    glm_normalize(movement_mat.col[2].raw);

    vec3s desired_vel = glms_mat3_mulv(movement_mat, move_vec);
    cam_vel = glms_vec3_lerp(cam_vel, desired_vel, 0.2f);

    cam_pos = glms_vec3_add(cam_pos, cam_vel);

    mat4s view = glms_look(cam_pos, cam_forward, (vec3s) {{ 0.0f, -1.0f, 0.0f }});
    
    push_constants.vertex.model_view_projection = glms_mat4_mul(projection, view);
    push_constants.fragment.camera_position = cam_pos;

}