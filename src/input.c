#include "input.h"
#include "vk/gfx_pipeline.h"
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/struct/cam.h>
#include <cglm/struct/mat3.h>
#include <stdbool.h>
#include <math.h>

#define M_TAU (GLM_PI * 2)

#define MOVE_SPEED 0.3f
#define ROT_SPEED 1.0f

static vec3s cam_pos = {{ 1.5f, -0.5f, 1.0f }};
static vec3s cam_vel = {{ 0.0f, 0.0f, 0.0f }};

static vec2s cam_rot = {{ -2.21f, 0.21f }};
static vec2s cam_rot_vel = {{ 0.0f, 0.0f }};

static bool has_last_norm_cursor_pos = false;
static vec2s last_norm_cursor_pos = {{ 0.0f, 0.0f }};

void handle_input(float delta) {
    float aspect = (float)swap_image_extent.width/(float)swap_image_extent.height;

    mat4s projection = glms_perspective(M_TAU / 5.0f, aspect, 0.01f, 300.0f);

    vec2s desired_rot_vel = {{ 0.0f, 0.0f }};
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS) {
        double cursor_x;
        double cursor_y;
        glfwGetCursorPos(window, &cursor_x, &cursor_y);

        const vec2s cursor_pos = {{ cursor_x, cursor_y }};
        if (cursor_pos.x >= 0 && cursor_pos.x <= swap_image_extent.width && cursor_pos.y >= 0.0f && cursor_y <= swap_image_extent.height) {
            vec2s norm_cursor_pos = glms_vec2_mul(cursor_pos, (vec2s) {{ 1.0f/(float)swap_image_extent.width, 1.0f/(float)swap_image_extent.height }});
            norm_cursor_pos.x *= aspect;

            if (has_last_norm_cursor_pos) {
                desired_rot_vel = glms_vec2_scale(glms_vec2_sub(norm_cursor_pos, last_norm_cursor_pos), ROT_SPEED);
                desired_rot_vel.y *= -1.0f;
            }

            has_last_norm_cursor_pos = true;
            last_norm_cursor_pos = norm_cursor_pos;
        } else {
            has_last_norm_cursor_pos = false;
        }
    } else {
        has_last_norm_cursor_pos = false;
    }
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
    vec3s cam_right = {{ -cam_forward.z, 0.0f, cam_forward.x }};
    glm_normalize(cam_right.raw);

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
        { -cam_forward.z, 0, cam_forward.x }
    }};
    glm_normalize(movement_mat.col[2].raw);

    vec3s desired_vel = glms_mat3_mulv(movement_mat, move_vec);
    cam_vel = glms_vec3_lerp(cam_vel, desired_vel, 0.2f);

    cam_pos = glms_vec3_add(cam_pos, cam_vel);

    mat4s view = glms_look(cam_pos, cam_forward, (vec3s) {{ 0.0f, 1.0f, 0.0f }});

    clip_space = glms_mat4_mul(projection, view);

}