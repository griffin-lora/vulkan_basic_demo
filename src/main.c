#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "util.h"

#define WIDTH 800
#define HEIGHT 600

static const char* layers[] = {
    "VK_LAYER_KHRONOS_validation"
};

static bool check_layers() {
    uint32_t num_available_layers;
    vkEnumerateInstanceLayerProperties(&num_available_layers, NULL);
    
    VkLayerProperties* available_layers = malloc(num_available_layers*sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&num_available_layers, available_layers);

    // TODO: This is kinda ugly
    for (size_t i = 0; i < NUM_ELEMS(layers); i++) {
        bool found = false;
        for (size_t j = 0; j < num_available_layers; j++) {
            if (strcmp(layers[i], available_layers[j].layerName) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            free(available_layers);
            return false;
        }
    }

    free(available_layers);
    return true;
}

static uint64_t get_graphics_queue_family_index(size_t num_queue_families, const VkQueueFamilyProperties queue_families[]) {
    for (size_t i = 0; i < num_queue_families; i++) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            return i;
        }
    }
    return -1;
}

static bool is_physical_device_suitable(VkPhysicalDevice physical_device) {
    // VkPhysicalDeviceProperties properties;
    // vkGetPhysicalDeviceProperties(device, &properties);

    // VkPhysicalDeviceFeatures features;
    // vkGetPhysicalDeviceFeatures(device, &features);

    // return properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

    uint32_t num_queue_families;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queue_families, NULL);

    VkQueueFamilyProperties* queue_families = malloc(num_queue_families*sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queue_families, queue_families);

    uint64_t graphics_queue_family_index = get_graphics_queue_family_index(num_queue_families, queue_families);

    free(queue_families);

    return graphics_queue_family_index != -1;
}

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* win = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", NULL, NULL);

    //
    if (!check_layers()) {
        printf("Validation layers requested, but not available\n");
        return 1;
    }

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0
    };

    uint32_t num_extensions;
    const char** extensions = glfwGetRequiredInstanceExtensions(&num_extensions);

    VkInstanceCreateInfo instance_create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = num_extensions,
        .ppEnabledExtensionNames = extensions,
        .enabledLayerCount = NUM_ELEMS(layers),
        .ppEnabledLayerNames = layers
        // RELEASE:
        // .enabledLayerCount = 0
    };
    VkInstance instance;
    if (vkCreateInstance(&instance_create_info, NULL, &instance) != VK_SUCCESS) {
        printf("Failed to create instance\n");
        return 1;
    }

    VkPhysicalDevice physical_device = VK_NULL_HANDLE;

    uint32_t num_physical_devices;
    vkEnumeratePhysicalDevices(instance, &num_physical_devices, NULL);
    if (num_physical_devices == 0) {
        printf("Failed to find physical devices with Vulkan support");
        return 1;
    }

    VkPhysicalDevice* physical_devices = malloc(num_physical_devices*sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &num_physical_devices, physical_devices);

    for (size_t i = 0; i < num_physical_devices; i++) {
        if (is_physical_device_suitable(physical_devices[i])) {
            physical_device = physical_devices[i];
            break;
        }
    }

    free(physical_devices);

    if (physical_device == VK_NULL_HANDLE) {
        printf("Failed to find a suitable physical device");
        return 1;
    }

    //

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        // draw frame
    }

    vkDestroyInstance(instance, NULL);

    glfwDestroyWindow(win);
    glfwTerminate();

    return 0;
}