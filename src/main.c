#include "vk/core.h"
#include "vk/render.h"
#include <stdbool.h>
#include <stdio.h>

int main() {
    const char* msg = init_vulkan_core();
    if (msg != NULL) {
        printf("%s", msg);
        return 1;
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        msg = draw_vulkan_frame();
        if (msg != NULL) {
            printf("%s", msg);
            return 1;
        }
    }

    term_vulkan_all();

    return 0;
}