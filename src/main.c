#include "vk/core.h"
#include "vk/render.h"
#include "input.h"
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
    const char* msg = init_vulkan_core();
    if (msg != NULL) {
        printf("%s", msg);
        return 1;
    }

    float last_delta = 1.0f/60.0f;
    while (!glfwWindowShouldClose(window)) {
        double start = glfwGetTime();
        glfwPollEvents();

        handle_input(last_delta);

        msg = draw_vulkan_frame();
        if (msg != NULL) {
            printf("%s", msg);
            return 1;
        }
        double end = glfwGetTime();
        double delta = end - start;

        if (delta > (1.0f/60.0f)) {
            printf("%f\n", delta);
        }

        double remaining = (1.0f/60.0f) - delta;
        if (remaining > 0.0f) {
            usleep(remaining * 1000000);
        }

        last_delta = delta;
    }

    term_vulkan_all();

    return 0;
}