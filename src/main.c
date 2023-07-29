#include <stdio.h>
#include <GLFW/glfw3.h>

static GLFWwindow* win;

#define WIDTH 800
#define HEIGHT 600

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    win = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", NULL, NULL);

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        // draw frame
    }

    glfwDestroyWindow(win);
    glfwTerminate();

    return 0;
}