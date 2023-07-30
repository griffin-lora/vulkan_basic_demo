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

static uint32_t get_graphics_queue_family_index(size_t num_queue_families, const VkQueueFamilyProperties queue_families[]) {
    for (size_t i = 0; i < num_queue_families; i++) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            return i;
        }
    }
    return NULL_UINT32;
}

static uint32_t get_presentation_queue_family_index(VkPhysicalDevice physical_device, VkSurfaceKHR surface, size_t num_queue_families, const VkQueueFamilyProperties queue_families[]) {
    for (size_t i = 0; i < num_queue_families; i++) {
        if (!(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            continue;
        }

        VkBool32 presentation_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &presentation_support);
        if (presentation_support) {
            return i;
        }
    }
    return NULL_UINT32;
}

typedef struct {
    VkPhysicalDevice physical_device;
    union {
        uint32_t data[2];
        struct {
            uint32_t graphics;
            uint32_t presentation;
        };
    } queue_family_indices;
} get_physical_device_t;

static get_physical_device_t get_physical_device(VkSurfaceKHR surface, size_t num_physical_devices, const VkPhysicalDevice physical_devices[]) {
    for (size_t i = 0; i < num_physical_devices; i++) {
        VkPhysicalDevice physical_device = physical_devices[i];

        uint32_t num_queue_families;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queue_families, NULL);

        VkQueueFamilyProperties* queue_families = malloc(num_queue_families*sizeof(VkQueueFamilyProperties));
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queue_families, queue_families);

        uint64_t graphics_queue_family_index = get_graphics_queue_family_index(num_queue_families, queue_families);
        uint64_t presentation_queue_family_index = get_presentation_queue_family_index(physical_device, surface, num_queue_families, queue_families);

        free(queue_families);

        if (graphics_queue_family_index == NULL_UINT32 || presentation_queue_family_index == NULL_UINT32) {
            continue;
        }

        return (get_physical_device_t) { physical_device, {{ graphics_queue_family_index, presentation_queue_family_index }} };
    }
    return (get_physical_device_t) { VK_NULL_HANDLE };
}

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", NULL, NULL);

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

    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance, window, NULL, &surface) != VK_SUCCESS) {
        printf("Failed to create window surface\n");
        return 1;
    }

    uint32_t num_physical_devices;
    vkEnumeratePhysicalDevices(instance, &num_physical_devices, NULL);
    if (num_physical_devices == 0) {
        printf("Failed to find physical devices with Vulkan support");
        return 1;
    }

    VkPhysicalDevice* physical_devices = malloc(num_physical_devices*sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &num_physical_devices, physical_devices);

    get_physical_device_t t0 = get_physical_device(surface, num_physical_devices, physical_devices);
    
    if (t0.physical_device == VK_NULL_HANDLE) {
        printf("Failed to find a suitable physical device");
        return 1;
    }

    free(physical_devices);
    
    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_infos[2];
    for (size_t i = 0; i < 2; i++) {
        queue_create_infos[i] = (VkDeviceQueueCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = t0.queue_family_indices.data[i],
            .queueCount = 1,
            .pQueuePriorities = &queue_priority
        };
    }

    VkPhysicalDeviceFeatures features = {};

    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = queue_create_infos,
        .pEnabledFeatures = &features,

        .enabledExtensionCount = 0,
        .enabledLayerCount = NUM_ELEMS(layers),
        .ppEnabledLayerNames = layers
    };

    VkDevice device;
    if (vkCreateDevice(t0.physical_device, &device_create_info, NULL, &device) != VK_SUCCESS) {
        printf("Failed to create logical device");
        return 1;
    }

    VkQueue graphics_queue;
    vkGetDeviceQueue(device, t0.queue_family_indices.graphics, 0, &graphics_queue);

    VkQueue presentation_queue;
    vkGetDeviceQueue(device, t0.queue_family_indices.presentation, 0, &presentation_queue);

    //

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        // draw frame
    }
    
    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}