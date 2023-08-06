#include "vk/core.h"
#include "vk/render.h"
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
    const char* msg = init_vulkan_core();
    if (msg != NULL) {
        printf("%s", msg);
        return 1;
    }

    while (!glfwWindowShouldClose(window)) {
        double start = glfwGetTime();
        glfwPollEvents();

        msg = draw_vulkan_frame();
        if (msg != NULL) {
            printf("%s", msg);
            return 1;
        }
        double end = glfwGetTime();
        double delta = end - start;

        double remaining = (1.0f/60.0f) - delta;
        if (remaining > 0.0f) {
            usleep(remaining * 1000000);
        }
    }

    term_vulkan_all();

    return 0;
}