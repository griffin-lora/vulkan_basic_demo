#include "vk/core.h"
#include "vk/render.h"
#include "input.h"
#include "chrono.h"
#include <stdbool.h>
#include <stdio.h>

int main(void) {
    const char* msg = init_vulkan_core();
    if (msg != NULL) {
        printf("%s", msg);
        return 1;
    }

    microseconds_t program_start = get_current_microseconds();

    float delta = 1.0f/60.0f;
    while (!glfwWindowShouldClose(window)) {
        microseconds_t start = get_current_microseconds() - program_start;
        glfwPollEvents();

        handle_input(delta);

        msg = draw_vulkan_frame();
        if (msg != NULL) {
            printf("%s", msg);
            return 1;
        }
        microseconds_t end = get_current_microseconds() - program_start;
        microseconds_t delta_microseconds = end - start;
        delta = (float)delta_microseconds/1000000.0f;

        if (delta > (1.0f/60.0f)) {
            printf("%f\n", delta);
        }

        microseconds_t remaining_microseconds = (1000000l/60l) - delta_microseconds;
        if (remaining_microseconds > 0) {
            sleep_microseconds(remaining_microseconds);
        }
    }

    term_vulkan_all();

    return 0;
}