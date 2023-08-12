#include "core.h"
#include "ds.h"
#include "util.h"
#include "result.h"
#include "gfx_pipeline.h"
#include "gfx_core.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#define WIDTH 640
#define HEIGHT 480

static const char* layers[] = {
    "VK_LAYER_KHRONOS_validation"
};

static const char* extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static result_t check_layers(void) {
    uint32_t num_available_layers;
    vkEnumerateInstanceLayerProperties(&num_available_layers, NULL);
    
    VkLayerProperties* available_layers = ds_promise(num_available_layers*sizeof(VkLayerProperties));
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
    
    VkExtensionProperties* available_extensions = ds_promise(num_available_extensions*sizeof(VkExtensionProperties));
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

static uint32_t get_graphics_queue_family_index(size_t num_queue_families, const VkQueueFamilyProperties queue_families[]) {
    for (size_t i = 0; i < num_queue_families; i++) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            return i;
        }
    }
    return NULL_UINT32;
}

static uint32_t get_presentation_queue_family_index(VkPhysicalDevice physical_device, size_t num_queue_families, const VkQueueFamilyProperties queue_families[]) {
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

static result_t get_physical_device(size_t num_physical_devices, const VkPhysicalDevice physical_devices[], VkPhysicalDevice* out_physical_device, uint32_t* out_num_surface_formats, uint32_t* out_num_present_modes, queue_family_indices_t* out_queue_family_indices) {
    for (size_t i = 0; i < num_physical_devices; i++) {
        VkPhysicalDevice physical_device = physical_devices[i];

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

        VkQueueFamilyProperties* queue_families = ds_promise(num_queue_families*sizeof(VkQueueFamilyProperties));
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queue_families, queue_families);

        uint64_t graphics_queue_family_index = get_graphics_queue_family_index(num_queue_families, queue_families);
        if (graphics_queue_family_index == NULL_UINT32) {
            continue;
        }

        uint64_t presentation_queue_family_index = get_presentation_queue_family_index(physical_device, num_queue_families, queue_families);
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

static VkSurfaceFormatKHR get_surface_format(size_t num_surface_formats, const VkSurfaceFormatKHR surface_formats[]) {
    for (size_t i = 0; i < num_surface_formats; i++) {
        VkSurfaceFormatKHR surface_format = surface_formats[i];
        if (surface_format.format == VK_FORMAT_B8G8R8A8_SRGB && surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return surface_format;
        }
    }

    return surface_formats[0];
}

static VkPresentModeKHR get_present_mode(size_t num_present_modes, const VkPresentModeKHR present_modes[]) {
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

    int32_t width;
    int32_t height;
    glfwGetFramebufferSize(window, &width, &height);
    VkExtent2D extent = { width, height };

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

    VkSwapchainCreateInfoKHR swapchain_create_info = {
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
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices = queue_family_indices.data;
    } else {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices = NULL;
    }

    if (vkCreateSwapchainKHR(device, &swapchain_create_info, NULL, &swapchain) != VK_SUCCESS) {
        return result_failure;
    }

    return result_success;
}

static result_t init_color_image(void) {
    if (create_image(swap_image_extent.width, swap_image_extent.height, 1, surface_format.format, render_multisample_flags, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &color_image, &color_image_memory) != result_success) {
        return result_failure;
    }
    if (create_image_view(color_image, 1, surface_format.format, VK_IMAGE_ASPECT_COLOR_BIT, &color_image_view) != result_success) {
        return result_failure;
    }
    return result_success;
}

static result_t init_depth_image(void) {
    if (create_image(swap_image_extent.width, swap_image_extent.height, 1, depth_image_format, render_multisample_flags, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depth_image, &depth_image_memory) != result_success) {
        return result_failure;
    }
    if (create_image_view(depth_image, 1, depth_image_format, depth_image_format == VK_FORMAT_D32_SFLOAT_S8_UINT || depth_image_format == VK_FORMAT_D24_UNORM_S8_UINT ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_DEPTH_BIT, &depth_image_view) != result_success) {
        return result_failure;
    }
    return result_success;
}

static result_t init_swapchain_framebuffers(void) {
    vkGetSwapchainImagesKHR(device, swapchain, &num_swapchain_images, swapchain_images);

    for (size_t i = 0; i < num_swapchain_images; i++) {
        if (create_image_view(swapchain_images[i], 1, surface_format.format, VK_IMAGE_ASPECT_COLOR_BIT, &swapchain_image_views[i]) != result_success) {
            return result_failure;
        }
    }

    for (size_t i = 0; i < num_swapchain_images; i++) {
        VkImageView attachments[] = { color_image_view, depth_image_view, swapchain_image_views[i] };

        VkFramebufferCreateInfo framebuffer_create_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = render_pass,
            .attachmentCount = NUM_ELEMS(attachments),
            .pAttachments = attachments,
            .width = swap_image_extent.width,
            .height = swap_image_extent.height,
            .layers = 1
        };

        if (vkCreateFramebuffer(device, &framebuffer_create_info, NULL, &swapchain_framebuffers[i]) != VK_SUCCESS) {
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

static void term_color_image(void) {
    vkDestroyImageView(device, color_image_view, NULL);
    vkDestroyImage(device, color_image, NULL);
    vkFreeMemory(device, color_image_memory, NULL);
}

static void term_depth_image(void) {
    vkDestroyImageView(device, depth_image_view, NULL);
    vkDestroyImage(device, depth_image, NULL);
    vkFreeMemory(device, depth_image_memory, NULL);
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

    vkDeviceWaitIdle(device);
    
    term_color_image();
    term_depth_image();
    term_swapchain();
    init_swapchain();
    init_color_image();
    init_depth_image();
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

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0
    };

    uint32_t num_instance_extensions;
    const char** instance_extensions = glfwGetRequiredInstanceExtensions(&num_instance_extensions);

    VkInstanceCreateInfo instance_create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = num_instance_extensions,
        .ppEnabledExtensionNames = instance_extensions,
        .enabledLayerCount = NUM_ELEMS(layers),
        .ppEnabledLayerNames = layers
        // RELEASE:
        // .enabledLayerCount = 0
    };
    if (vkCreateInstance(&instance_create_info, NULL, &instance) != VK_SUCCESS) {
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

    VkPhysicalDevice* physical_devices = ds_push(num_physical_devices*sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &num_physical_devices, physical_devices);

    uint32_t num_surface_formats;
    uint32_t num_present_modes;
    if (get_physical_device(num_physical_devices, physical_devices, &physical_device, &num_surface_formats, &num_present_modes, &queue_family_indices) != result_success) {
        return "Failed to get a suitable physical device\n";
    }

    ds_restore(physical_devices);

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

    VkPhysicalDeviceFeatures features = {
        .samplerAnisotropy = VK_TRUE
    };

    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = queue_create_infos,
        .pEnabledFeatures = &features,

        .enabledExtensionCount = NUM_ELEMS(extensions),
        .ppEnabledExtensionNames = extensions,
        .enabledLayerCount = NUM_ELEMS(layers),
        .ppEnabledLayerNames = layers
    };

    if (vkCreateDevice(physical_device, &device_create_info, NULL, &device) != VK_SUCCESS) {
        return "Failed to create logical device\n";
    }

    VkSemaphoreCreateInfo semaphore_create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO  
    };

    VkFenceCreateInfo fence_create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    for (size_t i = 0; i < NUM_FRAMES_IN_FLIGHT; i++) {
        if (
            vkCreateSemaphore(device, &semaphore_create_info, NULL, &image_available_semaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphore_create_info, NULL, &render_finished_semaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fence_create_info, NULL, &in_flight_fences[i]) != VK_SUCCESS
        ) {
            return "Failed to create synchronization primitives\n";
        }
    }

    vkGetDeviceQueue(device, queue_family_indices.graphics, 0, &graphics_queue);
    vkGetDeviceQueue(device, queue_family_indices.presentation, 0, &presentation_queue);

    {
        VkSurfaceFormatKHR* surface_formats = ds_promise(num_surface_formats*sizeof(VkSurfaceFormatKHR));
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_surface_formats, surface_formats);

        surface_format = get_surface_format(num_surface_formats, surface_formats);
    }

    {
        VkPresentModeKHR* present_modes = ds_promise(num_present_modes*sizeof(VkPresentModeKHR));
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &num_present_modes, present_modes);

        present_mode = get_present_mode(num_present_modes, present_modes);
    }

    if (init_swapchain() != result_success) {
        return "Failed to create swap chain\n";
    }

    VkCommandPoolCreateInfo command_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queue_family_indices.graphics
    };

    if (vkCreateCommandPool(device, &command_pool_create_info, NULL, &command_pool) != VK_SUCCESS) {
        return "Failed to create command pool\n";
    }

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = NUM_FRAMES_IN_FLIGHT
    };

    if (vkAllocateCommandBuffers(device, &command_buffer_allocate_info, render_command_buffers) != VK_SUCCESS) {
        return "Failed to allocate command buffers\n";
    }

    if (init_color_image() != result_success) {
        return "Failed to create color image\n";
    }
    
    {
        VkFormat formats[] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
        depth_image_format = get_supported_format(NUM_ELEMS(formats), formats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }
    if (depth_image_format == VK_FORMAT_MAX_ENUM) {
        return "Failed to get a supported depth image format\n";
    }
    
    if (init_depth_image() != result_success) {
        return "Failed to create depth image\n";
    }

    const char* msg = init_vulkan_graphics_pipeline(&physical_device_properties);
    if (msg != NULL) {
        return msg;
    }

    vkGetSwapchainImagesKHR(device, swapchain, &num_swapchain_images, NULL);
    swapchain_images = ds_push(num_swapchain_images*sizeof(VkImage));
    swapchain_image_views = ds_push(num_swapchain_images*sizeof(VkImageView));
    swapchain_framebuffers = ds_push(num_swapchain_images*sizeof(VkFramebuffer));

    if (init_swapchain_framebuffers() != result_success) {
        return "Failed to create framebuffer\n";
    }

    return NULL;
}

void term_vulkan_all(void) {
    vkDeviceWaitIdle(device);

    vkDestroyCommandPool(device, command_pool, NULL);

    vkDestroyPipeline(device, pipeline, NULL);
    vkDestroyPipelineLayout(device, pipeline_layout, NULL);

    vkDestroyRenderPass(device, render_pass, NULL);
    
    term_color_image();
    term_depth_image();
    term_swapchain();

    for (size_t i = 0; i < NUM_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, image_available_semaphores[i], NULL);
        vkDestroySemaphore(device, render_finished_semaphores[i], NULL);
        vkDestroyFence(device, in_flight_fences[i], NULL);
        vkDestroyBuffer(device, clip_space_uniform_buffers[i], NULL);
        vkFreeMemory(device, clip_space_uniform_buffers_memory[i], NULL);
    }

    vkDestroyDescriptorPool(device, descriptor_pool, NULL);
    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, NULL);

    vkDestroySampler(device, texture_image_sampler, NULL);
    vkDestroyImageView(device, texture_image_view, NULL);

    vkDestroyImage(device, texture_image, NULL);
    vkFreeMemory(device, texture_image_memory, NULL);
    vkDestroyBuffer(device, vertex_buffer, NULL);
    vkFreeMemory(device, vertex_buffer_memory, NULL);
    vkDestroyBuffer(device, index_buffer, NULL);
    vkFreeMemory(device, index_buffer_memory, NULL);

    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);

    glfwDestroyWindow(window);
    glfwTerminate();
}