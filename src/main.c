#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "util.h"

#define WIDTH 800
#define HEIGHT 600

static const char* layers[] = {
    "VK_LAYER_KHRONOS_validation"
};

static const char* extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static bool check_layers() {
    uint32_t num_available_layers;
    vkEnumerateInstanceLayerProperties(&num_available_layers, NULL);
    
    VkLayerProperties* available_layers = malloc(num_available_layers*sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&num_available_layers, available_layers);

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

static bool check_extensions(VkPhysicalDevice physical_device) {
    uint32_t num_available_extensions;
    vkEnumerateDeviceExtensionProperties(physical_device, NULL, &num_available_extensions, NULL);
    
    VkExtensionProperties* available_extensions = malloc(num_available_extensions*sizeof(VkExtensionProperties));
    vkEnumerateDeviceExtensionProperties(physical_device, NULL, &num_available_extensions, available_extensions);

    // TODO: This is kinda ugly
    for (size_t i = 0; i < NUM_ELEMS(extensions); i++) {
        bool found = false;
        for (size_t j = 0; j < num_available_extensions; j++) {
            if (strcmp(extensions[i], available_extensions[j].extensionName) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            free(available_extensions);
            return false;
        }
    }

    free(available_extensions);
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
    uint32_t num_surface_formats;
    uint32_t num_present_modes;
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
        
        if (!check_extensions(physical_device)) {
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

        VkQueueFamilyProperties* queue_families = malloc(num_queue_families*sizeof(VkQueueFamilyProperties));
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queue_families, queue_families);

        uint64_t graphics_queue_family_index = get_graphics_queue_family_index(num_queue_families, queue_families);
        if (graphics_queue_family_index == NULL_UINT32) {
            free(queue_families);
            continue;
        }

        uint64_t presentation_queue_family_index = get_presentation_queue_family_index(physical_device, surface, num_queue_families, queue_families);
        if (presentation_queue_family_index == NULL_UINT32) {
            free(queue_families);
            break;
        }

        free(queue_families);

        return (get_physical_device_t) { physical_device, num_surface_formats, num_present_modes, {{ graphics_queue_family_index, presentation_queue_family_index }} };
    }
    return (get_physical_device_t) { VK_NULL_HANDLE };
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

static VkExtent2D get_swap_extent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR* capabilities) {
    if (capabilities->currentExtent.width != NULL_UINT32) {
        return capabilities->currentExtent;
    }

    int32_t width;
    int32_t height;
    glfwGetFramebufferSize(window, &width, &height);
    VkExtent2D extent = { width, height };

    extent.width = clamp_uint32(extent.width, capabilities->maxImageExtent.width, capabilities->maxImageExtent.width);
    extent.height = clamp_uint32(extent.height, capabilities->maxImageExtent.height, capabilities->maxImageExtent.height);

    return extent;
}

typedef struct {
    union {
        uint32_t error;
        uint32_t num_bytes;
    };
    uint32_t* bytes;
} shader_bytecode_t;

static shader_bytecode_t read_shader_bytecode(const char* path) {
    if (access(path, F_OK) != 0) {
        return (shader_bytecode_t) { { .error = NULL_UINT32 } };
    }

    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return (shader_bytecode_t) { { .error = NULL_UINT32 } };
    }

    struct stat st;
    stat(path, &st);

    uint32_t aligned_num_bytes = st.st_size % 32 == 0 ? st.st_size : st.st_size + (32 - (st.st_size % 32));

    uint32_t* bytes = malloc(aligned_num_bytes);
    memset(bytes, 0, aligned_num_bytes);
    if (fread(bytes, st.st_size, 1, file) != 1) {
        return (shader_bytecode_t) { { .error = NULL_UINT32 } };
    }

    fclose(file);

    return (shader_bytecode_t) { { .num_bytes = st.st_size }, .bytes = bytes };
}

static VkShaderModule init_shader_module(VkDevice device, const shader_bytecode_t bytecode) {
    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = bytecode.num_bytes,
        .pCode = bytecode.bytes
    };

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device, &create_info, NULL, &shader_module) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return shader_module;
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

    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(t0.physical_device, &physical_device_properties);
    printf("Loaded physical device \"%s\"\n", physical_device_properties.deviceName);
    
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

        .enabledExtensionCount = NUM_ELEMS(extensions),
        .ppEnabledExtensionNames = extensions,
        .enabledLayerCount = NUM_ELEMS(layers),
        .ppEnabledLayerNames = layers
    };

    VkDevice device;
    if (vkCreateDevice(t0.physical_device, &device_create_info, NULL, &device) != VK_SUCCESS) {
        printf("Failed to create logical device\n");
        return 1;
    }

    VkQueue graphics_queue;
    vkGetDeviceQueue(device, t0.queue_family_indices.graphics, 0, &graphics_queue);

    VkQueue presentation_queue;
    vkGetDeviceQueue(device, t0.queue_family_indices.presentation, 0, &presentation_queue);

    VkSurfaceFormatKHR* surface_formats = malloc(t0.num_surface_formats*sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(t0.physical_device, surface, &t0.num_surface_formats, surface_formats);

    VkSurfaceFormatKHR surface_format = get_surface_format(t0.num_surface_formats, surface_formats);

    free(surface_formats);

    VkPresentModeKHR* present_modes = malloc(t0.num_present_modes*sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR(t0.physical_device, surface, &t0.num_present_modes, present_modes);

    VkPresentModeKHR present_mode = get_present_mode(t0.num_present_modes, present_modes);

    free(present_modes);

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(t0.physical_device, surface, &capabilities);
    VkExtent2D swap_extent = get_swap_extent(window, &capabilities);

    uint32_t min_num_images = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0) {
        min_num_images = clamp_uint32(min_num_images, 0, capabilities.maxImageCount);
    }

    VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = min_num_images,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = swap_extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    if (t0.queue_family_indices.graphics != t0.queue_family_indices.presentation) {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices = t0.queue_family_indices.data;
    } else {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices = NULL;
    }

    VkSwapchainKHR swapchain;
    if (vkCreateSwapchainKHR(device, &swapchain_create_info, NULL, &swapchain) != VK_SUCCESS) {
        printf("Failed to create swap chain\n");
        return 1;
    }

    uint32_t num_images;
    vkGetSwapchainImagesKHR(device, swapchain, &num_images, NULL);

    VkImage* images = malloc(num_images*sizeof(VkImage));
    vkGetSwapchainImagesKHR(device, swapchain, &num_images, images);

    VkImageView* image_views = malloc(num_images*sizeof(VkImageView));
    for (size_t i = 0; i < num_images; i++) {
        VkImageViewCreateInfo image_view_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = surface_format.format,
            .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1
        };
        vkCreateImageView(device, &image_view_create_info, NULL, &image_views[i]);
    }

    VkAttachmentDescription color_attachment = {
        .format = surface_format.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference color_attachment_reference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_reference
    };

    VkRenderPassCreateInfo render_pass_create_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass
    };

    VkRenderPass render_pass;
    if (vkCreateRenderPass(device, &render_pass_create_info, NULL, &render_pass) != VK_SUCCESS) {
        printf("Failed to create render pass\n");
        return 1;
    }

    const shader_bytecode_t vertex_shader_bytecode = read_shader_bytecode("shader/vertex.spv");
    if (vertex_shader_bytecode.error == NULL_UINT32) {
        printf("Failed to load vertex shader\n");
        return 1;
    }

    const shader_bytecode_t fragment_shader_bytecode = read_shader_bytecode("shader/fragment.spv");
    if (vertex_shader_bytecode.error == NULL_UINT32) {
        printf("Failed to load fragment shader\n");
        return 1;
    }

    VkShaderModule vertex_shader_module = init_shader_module(device, vertex_shader_bytecode);
    if (vertex_shader_module == VK_NULL_HANDLE) {
        printf("Failed to create vertex shader module\n");
    }

    VkShaderModule fragment_shader_module = init_shader_module(device, fragment_shader_bytecode);
    if (fragment_shader_module == VK_NULL_HANDLE) {
        printf("Failed to create vertex shader module\n");
    }
    
    free(vertex_shader_bytecode.bytes);
    free(fragment_shader_bytecode.bytes);

    VkPipelineShaderStageCreateInfo shader_pipeline_stage_create_infos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertex_shader_module,
            .pName = "main",
            .pSpecializationInfo = NULL
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragment_shader_module,
            .pName = "main",
            .pSpecializationInfo = NULL
        }
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_pipeline_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = NULL,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = NULL,
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_pipeline_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkPipelineViewportStateCreateInfo viewport_pipeline_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    VkPipelineRasterizationStateCreateInfo rasterization_pipeline_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL, // solid geometry, not wireframe
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE
    };

    VkPipelineMultisampleStateCreateInfo multisample_pipeline_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment_pipeline_state_create_info = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE
    };

    VkPipelineColorBlendStateCreateInfo color_blend_pipeline_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment_pipeline_state_create_info
    };

    VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamic_pipeline_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = NUM_ELEMS(dynamic_states),
        .pDynamicStates = dynamic_states
    };

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pushConstantRangeCount = 0
    };

    VkPipelineLayout pipeline_layout;
    if (vkCreatePipelineLayout(device, &pipeline_layout_create_info, NULL, &pipeline_layout) != VK_SUCCESS) {
        printf("Failed to create pipeline layout");
        return 1;
    }

    VkGraphicsPipelineCreateInfo pipeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shader_pipeline_stage_create_infos,
        .pVertexInputState = &vertex_input_pipeline_state_create_info,
        .pInputAssemblyState = &input_assembly_pipeline_state_create_info,
        .pViewportState = &viewport_pipeline_state_create_info,
        .pRasterizationState = &rasterization_pipeline_state_create_info,
        .pMultisampleState = &multisample_pipeline_state_create_info,
        .pDepthStencilState = NULL,
        .pColorBlendState = &color_blend_pipeline_state_create_info,
        .pDynamicState = &dynamic_pipeline_state_create_info,
        .layout = pipeline_layout,
        .renderPass = render_pass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &pipeline) != VK_SUCCESS) {
        printf("Failed to create graphics pipeline\n");
        return 1;
    }

    vkDestroyShaderModule(device, vertex_shader_module, NULL);
    vkDestroyShaderModule(device, fragment_shader_module, NULL);

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = swap_extent.width,
        .height = swap_extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VkRect2D scissor = {
        .offset = { 0, 0 },
        .extent = swap_extent
    };

    //

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        // draw frame
    }

    vkDestroyPipeline(device, pipeline, NULL);
    vkDestroyPipelineLayout(device, pipeline_layout, NULL);
    vkDestroyRenderPass(device, render_pass, NULL);

    for (size_t i = 0; i < num_images; i++) {
        vkDestroyImageView(device, image_views[i], NULL);
    }
    
    vkDestroySwapchainKHR(device, swapchain, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);

    glfwDestroyWindow(window);
    glfwTerminate();

    free(image_views);
    free(images);

    return 0;
}