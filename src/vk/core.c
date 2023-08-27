#include "core.h"
#include "util.h"
#include "result.h"
#include "gfx_pipeline.h"
#include "shadow_pipeline.h"
#include "color_pipeline.h"
#include "asset.h"
#include "gfx_core.h"
#include "defaults.h"
#include <stdbool.h>
#include <string.h>
#include <malloc.h>
#include <stdalign.h>
#include <stdio.h>

#define WIDTH 640
#define HEIGHT 480

alignas(64)
GLFWwindow* window;
VkDevice device;
VkPhysicalDevice physical_device;
VmaAllocator allocator;
queue_family_indices_t queue_family_indices;
VkSurfaceFormatKHR surface_format;
VkPresentModeKHR present_mode;
VkSemaphore image_available_semaphores[NUM_FRAMES_IN_FLIGHT];
VkSemaphore render_finished_semaphores[NUM_FRAMES_IN_FLIGHT];
VkFence in_flight_fences[NUM_FRAMES_IN_FLIGHT];
VkCommandPool command_pool;
uint32_t num_swapchain_images;
VkImage* swapchain_images;
VkImageView* swapchain_image_views;
VkFramebuffer* swapchain_framebuffers;
VkSwapchainKHR swapchain;
VkInstance instance;
VkSurfaceKHR surface;
VkExtent2D swap_image_extent;
VkQueue graphics_queue;
VkQueue presentation_queue;
bool framebuffer_resized;

VkSampleCountFlagBits render_multisample_flags;

VkFormat depth_image_format;

static const char* layers[] = {
    "VK_LAYER_KHRONOS_validation"
};

static const char* extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static result_t check_layers(void) {
    uint32_t num_available_layers;
    vkEnumerateInstanceLayerProperties(&num_available_layers, NULL);
    
    VkLayerProperties available_layers[num_available_layers];
    vkEnumerateInstanceLayerProperties(&num_available_layers, available_layers);

    for (size_t i = 0; i < NUM_ELEMS(layers); i++) {
        bool not_found = true;
        for (size_t j = 0; j < num_available_layers; j++) {
            if (strcmp(layers[i], available_layers[j].layerName) == 0) {
                not_found = false;
                break;
            }
        }

        if (not_found) {
            return result_failure;
        }
    }

    return result_success;
}

static result_t check_extensions(VkPhysicalDevice physical_device) {
    uint32_t num_available_extensions;
    vkEnumerateDeviceExtensionProperties(physical_device, NULL, &num_available_extensions, NULL);
    
    VkExtensionProperties available_extensions[num_available_extensions];
    vkEnumerateDeviceExtensionProperties(physical_device, NULL, &num_available_extensions, available_extensions);

    for (size_t i = 0; i < NUM_ELEMS(extensions); i++) {
        bool not_found = true;
        for (size_t j = 0; j < num_available_extensions; j++) {
            if (strcmp(extensions[i], available_extensions[j].extensionName) == 0) {
                not_found = false;
                break;
            }
        }

        if (not_found) {
            return result_failure;
        }
    }

    return result_success;
}

static uint32_t get_graphics_queue_family_index(uint32_t num_queue_families, const VkQueueFamilyProperties queue_families[]) {
    for (uint32_t i = 0; i < num_queue_families; i++) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            return i;
        }
    }
    return NULL_UINT32;
}

static uint32_t get_presentation_queue_family_index(VkPhysicalDevice physical_device, uint32_t num_queue_families, const VkQueueFamilyProperties queue_families[]) {
    for (uint32_t i = 0; i < num_queue_families; i++) {
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

static result_t get_physical_device(uint32_t num_physical_devices, const VkPhysicalDevice physical_devices[], VkPhysicalDevice* out_physical_device, uint32_t* out_num_surface_formats, uint32_t* out_num_present_modes, queue_family_indices_t* out_queue_family_indices) {
    for (size_t i = 0; i < num_physical_devices; i++) {
        VkPhysicalDevice physical_device = physical_devices[i];

        // RenderDoc loads llvmpipe for some reason so this forces it to load the correct physical device
        VkPhysicalDeviceProperties physical_device_properties;
        vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
        if (physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU || physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU || physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_OTHER) {
            continue;
        }

        VkFormatProperties format_properties;
        vkGetPhysicalDeviceFormatProperties(physical_device, VK_FORMAT_R8G8B8_SRGB, &format_properties);
        if (!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            continue;
        }

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(physical_device, &features);

        if (!features.samplerAnisotropy) {
            continue;
        }
        
        if (check_extensions(physical_device) != result_success) {
            continue;
        }
        
        uint32_t num_surface_formats;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_surface_formats, NULL);

        if (num_surface_formats == 0) {
            continue;
        }

        uint32_t num_present_modes;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &num_present_modes, NULL);

        if (num_present_modes == 0) {
            continue;
        }

        uint32_t num_queue_families;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queue_families, NULL);

        VkQueueFamilyProperties queue_families[num_queue_families];
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queue_families, queue_families);

        uint32_t graphics_queue_family_index = get_graphics_queue_family_index(num_queue_families, queue_families);
        if (graphics_queue_family_index == NULL_UINT32) {
            continue;
        }

        uint32_t presentation_queue_family_index = get_presentation_queue_family_index(physical_device, num_queue_families, queue_families);
        if (presentation_queue_family_index == NULL_UINT32) {
            break;
        }

        *out_physical_device = physical_device;
        *out_num_surface_formats = num_surface_formats;
        *out_num_present_modes = num_present_modes;
        *out_queue_family_indices = (queue_family_indices_t) {{ graphics_queue_family_index, presentation_queue_family_index }};
        return result_success;
    }
    return result_failure;
}

static VkSurfaceFormatKHR get_surface_format(uint32_t num_surface_formats, const VkSurfaceFormatKHR surface_formats[]) {
    for (size_t i = 0; i < num_surface_formats; i++) {
        VkSurfaceFormatKHR surface_format = surface_formats[i];
        if (surface_format.format == VK_FORMAT_B8G8R8A8_SRGB && surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return surface_format;
        }
    }

    return surface_formats[0];
}

static VkPresentModeKHR get_present_mode(uint32_t num_present_modes, const VkPresentModeKHR present_modes[]) {
    for (size_t i = 0; i < num_present_modes; i++) {
        VkPresentModeKHR present_mode = present_modes[i];
        if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return present_mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D get_swap_image_extent(const VkSurfaceCapabilitiesKHR* capabilities) {
    if (capabilities->currentExtent.width != NULL_UINT32) {
        return capabilities->currentExtent;
    }

    int width;
    int height;
    glfwGetFramebufferSize(window, &width, &height);
    VkExtent2D extent = { (uint32_t)width, (uint32_t)height };

    extent.width = clamp_uint32(extent.width, capabilities->minImageExtent.width, capabilities->maxImageExtent.width);
    extent.height = clamp_uint32(extent.height, capabilities->minImageExtent.height, capabilities->maxImageExtent.height);

    return extent;
}

static result_t init_swapchain(void) {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities);
    swap_image_extent = get_swap_image_extent(&capabilities); // NOTE: Not actually a problem? (https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/1340)

    uint32_t min_num_swapchain_images = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0) {
        min_num_swapchain_images = clamp_uint32(min_num_swapchain_images, 0, capabilities.maxImageCount);
    }

    VkSwapchainCreateInfoKHR info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = min_num_swapchain_images,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = swap_image_extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    if (queue_family_indices.graphics != queue_family_indices.presentation) {
        info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = 2;
        info.pQueueFamilyIndices = queue_family_indices.data;
    } else {
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.queueFamilyIndexCount = 0;
        info.pQueueFamilyIndices = NULL;
    }

    if (vkCreateSwapchainKHR(device, &info, NULL, &swapchain) != VK_SUCCESS) {
        return result_failure;
    }

    return result_success;
}

static result_t init_swapchain_framebuffers(void) {
    vkGetSwapchainImagesKHR(device, swapchain, &num_swapchain_images, swapchain_images);

    for (size_t i = 0; i < num_swapchain_images; i++) {
        if (vkCreateImageView(device, &(VkImageViewCreateInfo) {
            DEFAULT_VK_IMAGE_VIEW,
            .image = swapchain_images[i],
            .format = surface_format.format,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
        }, NULL, &swapchain_image_views[i]) != VK_SUCCESS) {
            return result_failure;
        }
    }

    for (size_t i = 0; i < num_swapchain_images; i++) {
        if (vkCreateFramebuffer(device, &(VkFramebufferCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = color_pipeline_render_pass,
            .attachmentCount = 3,
            .pAttachments = (VkImageView[3]) { color_image_view, depth_image_view, swapchain_image_views[i] },
            .width = swap_image_extent.width,
            .height = swap_image_extent.height,
            .layers = 1
        }, NULL, &swapchain_framebuffers[i]) != VK_SUCCESS) {
            return result_failure;
        }
    }
    return result_success;
}

static void term_swapchain(void) {
    for (size_t i = 0; i < num_swapchain_images; i++) {
        vkDestroyImageView(device, swapchain_image_views[i], NULL);
        vkDestroyFramebuffer(device, swapchain_framebuffers[i], NULL);
    }
    
    vkDestroySwapchainKHR(device, swapchain, NULL);
}

void reinit_swapchain(void) {
    // Even the tutorials use bad hacks, im all in
    int width;
    int height;
    glfwGetFramebufferSize(window, &width, &height);

    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    //
    
    term_color_pipeline_swapchain_dependents();
    term_swapchain();
    init_swapchain();
    init_color_pipeline_swapchain_dependents();
    init_swapchain_framebuffers();
}

static void framebuffer_resize(GLFWwindow*, int, int) {
    framebuffer_resized = true;
}

static VkFormat get_supported_format(size_t num_formats, const VkFormat formats[], VkImageTiling tiling, VkFormatFeatureFlags feature_flags) {
    for (size_t i = 0; i < num_formats; i++) {
        VkFormat format = formats[i];

        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(physical_device, format, &properties);

        if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & feature_flags) == feature_flags) {
            return format;
        }

        if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & feature_flags) == feature_flags) {
            return format;
        }
    }

    return VK_FORMAT_MAX_ENUM;
}

static VkSampleCountFlagBits get_max_multisample_flags(const VkPhysicalDeviceProperties* properties) {
    VkSampleCountFlags flags = properties->limits.framebufferColorSampleCounts & properties->limits.framebufferDepthSampleCounts;

    // Way too overkill for this project
    // if (flags & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    // if (flags & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    // if (flags & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (flags & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (flags & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (flags & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

const char* init_vulkan_core(void) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", NULL, NULL);
    glfwSetFramebufferSizeCallback(window, framebuffer_resize);

    //
    if (check_layers() != result_success) {
        return "Validation layers requested, but not available\n";
    }

    uint32_t num_instance_extensions;
    const char** instance_extensions = glfwGetRequiredInstanceExtensions(&num_instance_extensions);

    if (vkCreateInstance(&(VkInstanceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &(VkApplicationInfo) {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Hello Triangle",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_0
        },
        .enabledExtensionCount = num_instance_extensions,
        .ppEnabledExtensionNames = instance_extensions,
        .enabledLayerCount = NUM_ELEMS(layers),
        .ppEnabledLayerNames = layers
    }, NULL, &instance) != VK_SUCCESS) {
        return "Failed to create instance\n";
    }

    if (glfwCreateWindowSurface(instance, window, NULL, &surface) != VK_SUCCESS) {
        return "Failed to create window surface\n";
    }

    uint32_t num_physical_devices;
    vkEnumeratePhysicalDevices(instance, &num_physical_devices, NULL);
    if (num_physical_devices == 0) {
        return "Failed to find physical devices with Vulkan support\n";
    }

    uint32_t num_surface_formats;
    uint32_t num_present_modes;

    {
        VkPhysicalDevice physical_devices[num_physical_devices];
        vkEnumeratePhysicalDevices(instance, &num_physical_devices, physical_devices);

        if (get_physical_device(num_physical_devices, physical_devices, &physical_device, &num_surface_formats, &num_present_modes, &queue_family_indices) != result_success) {
            return "Failed to get a suitable physical device\n";
        }
    }

    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
    printf("Loaded physical device \"%s\"\n", physical_device_properties.deviceName);

    render_multisample_flags = get_max_multisample_flags(&physical_device_properties);

    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_infos[2];
    for (size_t i = 0; i < 2; i++) {
        queue_create_infos[i] = (VkDeviceQueueCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queue_family_indices.data[i],
            .queueCount = 1,
            .pQueuePriorities = &queue_priority
        };
    }

    if (vkCreateDevice(physical_device, &(VkDeviceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = queue_create_infos,
        .pEnabledFeatures = &(VkPhysicalDeviceFeatures) {
            .samplerAnisotropy = VK_TRUE
        },

        .enabledExtensionCount = NUM_ELEMS(extensions),
        .ppEnabledExtensionNames = extensions,
        .enabledLayerCount = NUM_ELEMS(layers),
        .ppEnabledLayerNames = layers
    }, NULL, &device) != VK_SUCCESS) {
        return "Failed to create logical device\n";
    }

    if (vmaCreateAllocator(&(VmaAllocatorCreateInfo) {
        .instance = instance,
        .physicalDevice = physical_device,
        .device = device,
        .pAllocationCallbacks = NULL,
        .pDeviceMemoryCallbacks = NULL,
        .vulkanApiVersion = VK_API_VERSION_1_0,
        .flags = 0 // Don't think any are needed
    }, &allocator) != VK_SUCCESS) {
        return "Failed to create memory allocator\n";
    }

    for (size_t i = 0; i < NUM_FRAMES_IN_FLIGHT; i++) {
        if (
            vkCreateSemaphore(device, &(VkSemaphoreCreateInfo) { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }, NULL, &image_available_semaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &(VkSemaphoreCreateInfo) { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }, NULL, &render_finished_semaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &(VkFenceCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT
            }, NULL, &in_flight_fences[i]) != VK_SUCCESS
        ) {
            return "Failed to create synchronization primitives\n";
        }
    }

    vkGetDeviceQueue(device, queue_family_indices.graphics, 0, &graphics_queue);
    vkGetDeviceQueue(device, queue_family_indices.presentation, 0, &presentation_queue);

    {
        VkSurfaceFormatKHR surface_formats[num_surface_formats];
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_surface_formats, surface_formats);

        surface_format = get_surface_format(num_surface_formats, surface_formats);
    }

    {
        VkPresentModeKHR present_modes[num_present_modes];
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &num_present_modes, present_modes);

        present_mode = get_present_mode(num_present_modes, present_modes);
    }

    if (init_swapchain() != result_success) {
        return "Failed to create swap chain\n";
    }

    if (vkCreateCommandPool(device, &(VkCommandPoolCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queue_family_indices.graphics
    }, NULL, &command_pool) != VK_SUCCESS) {
        return "Failed to create command pool\n";
    }
    
    depth_image_format = get_supported_format(3, (VkFormat[3]) { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    if (depth_image_format == VK_FORMAT_MAX_ENUM) {
        return "Failed to get a supported depth image format\n";
    }
    
    const char* msg = init_vulkan_assets(&physical_device_properties);
    if (msg != NULL) { return msg; }

    msg = init_vulkan_graphics_pipelines();
    if (msg != NULL) { return msg; }

    msg = draw_shadow_pipeline();
    if (msg != NULL) { return msg; }

    vkGetSwapchainImagesKHR(device, swapchain, &num_swapchain_images, NULL);
    swapchain_images = memalign(64, num_swapchain_images*sizeof(VkImage));
    swapchain_image_views = memalign(64, num_swapchain_images*sizeof(VkImageView));
    swapchain_framebuffers = memalign(64, num_swapchain_images*sizeof(VkFramebuffer));

    if (init_swapchain_framebuffers() != result_success) {
        return "Failed to create framebuffer\n";
    }

    return NULL;
}

void term_vulkan_all(void) {
    vkDeviceWaitIdle(device);

    vkDestroyCommandPool(device, command_pool, NULL);

    term_vulkan_graphics_pipelines();
    
    term_swapchain();

    for (size_t i = 0; i < NUM_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, image_available_semaphores[i], NULL);
        vkDestroySemaphore(device, render_finished_semaphores[i], NULL);
        vkDestroyFence(device, in_flight_fences[i], NULL);
    }

    term_vulkan_assets();

    vmaDestroyAllocator(allocator);

    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);

    free(swapchain_images);
    free(swapchain_image_views);
    free(swapchain_framebuffers);

    glfwDestroyWindow(window);
    glfwTerminate();
}