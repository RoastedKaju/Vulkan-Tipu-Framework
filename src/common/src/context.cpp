#include "context.h"
#include "utils.h"
#define VOLK_IMPLEMENTATION
#include <volk.h>
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>
#include <cassert>

// debug callback
// ReSharper disable once CppParameterMayBeConst
VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                              VkDebugUtilsMessageTypeFlagsEXT type,
                                              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                              void *pUserData) {
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::printf("[Validation] %s\n", pCallbackData->pMessage);
    }
    return VK_FALSE;
}

Context::Context() {
    Context(true);
}

Context::Context(const bool enable_validation_layers) : enable_validation_layers_(enable_validation_layers) {
    check(SDL_Init(SDL_INIT_VIDEO));
    check(SDL_Vulkan_LoadLibrary(nullptr));
    volkInitialize();
}

Context::~Context() {
}

bool Context::create_instance(const char *app_name) {
    VkApplicationInfo app_info{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = app_name,
        .apiVersion = VK_API_VERSION_1_3
    };

    uint32_t instance_extension_count{0};
    char const *const *sdl_extensions{SDL_Vulkan_GetInstanceExtensions(&instance_extension_count)};
    std::vector<const char *> instance_extensions(sdl_extensions, sdl_extensions + instance_extension_count);
    if (enable_validation_layers_) {
        instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        ++instance_extension_count;
    }

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debug_callback
    };

    const VkInstanceCreateInfo instance_create_info{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = &debug_create_info,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = enable_validation_layers_ ? static_cast<uint32_t>(validation_layers_.size()) : 0,
        .ppEnabledLayerNames = enable_validation_layers_ ? validation_layers_.data() : nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size()),
        .ppEnabledExtensionNames = instance_extensions.data()
    };

    check(vkCreateInstance(&instance_create_info, nullptr, &instance_));
    volkLoadInstance(instance_);

    if (enable_validation_layers_) {
        check(vkCreateDebugUtilsMessengerEXT(instance_, &debug_create_info, nullptr, &debug_messenger_));
    }

    return true;
}

bool Context::setup_device(const uint32_t device_index) {
    // device
    uint32_t device_count{0};
    check(vkEnumeratePhysicalDevices(instance_, &device_count, nullptr));
    std::vector<VkPhysicalDevice> devices(device_count);
    check(vkEnumeratePhysicalDevices(instance_, &device_count, devices.data()));

    device_index_ = device_index;
    assert(device_index_ < device_count);

    VkPhysicalDeviceProperties2 device_properties{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    vkGetPhysicalDeviceProperties2(devices[device_index], &device_properties);
    physical_device_ = devices[device_index];
    printf("Selected device: %s.\n", device_properties.properties.deviceName);

    // queue
    uint32_t queue_family_count{0};
    vkGetPhysicalDeviceQueueFamilyProperties(devices[device_index], &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(devices[device_index], &queue_family_count, queue_families.data());
    uint32_t queue_family_index{0};
    for (auto i = 0; i < queue_families.size(); ++i) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queue_family_index = static_cast<uint32_t>(i);
            break;
        }
    }
    check(SDL_Vulkan_GetPresentationSupport(instance_, devices[device_index], queue_family_index));

    // logical device
    constexpr float priorities = 1.0f;
    VkDeviceQueueCreateInfo queue_create_info{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = &priorities,
    };

    VkPhysicalDeviceVulkan12Features enabled_features_12{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .descriptorIndexing = true,
        .shaderSampledImageArrayNonUniformIndexing = true,
        .descriptorBindingPartiallyBound = true,
        .descriptorBindingVariableDescriptorCount = true,
        .runtimeDescriptorArray = true,
        .bufferDeviceAddress = true
    };
    VkPhysicalDeviceVulkan13Features enabled_features_13{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &enabled_features_12,
        .synchronization2 = true,
        .dynamicRendering = true
    };
    VkPhysicalDeviceFeatures enabled_features_10{
        .samplerAnisotropy = VK_TRUE,
        .shaderInt64 = VK_TRUE
    };

    const std::vector<const char *> device_extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo device_create_info{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &enabled_features_13,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_create_info,
        .enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
        .ppEnabledExtensionNames = device_extensions.data(),
        .pEnabledFeatures = &enabled_features_10
    };

    check(vkCreateDevice(devices[device_index], &device_create_info, nullptr, &device_));
    vkGetDeviceQueue(device_, queue_family_index, 0, &queue_);

    // VMA
    VmaVulkanFunctions vk_functions{
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
        .vkCreateImage = vkCreateImage
    };
    VmaAllocatorCreateInfo allocator_create_info{
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = devices[device_index],
        .device = device_,
        .pVulkanFunctions = &vk_functions,
        .instance = instance_
    };
    check(vmaCreateAllocator(&allocator_create_info, &allocator_));

    return true;
}

bool Context::create_window(const char *title, const uint32_t width, const uint32_t height) {
    SDL_Window *window = SDL_CreateWindow(title, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    assert(window && "Failed to create window");
    check(SDL_Vulkan_CreateSurface(window, instance_, nullptr, &surface_));
    check(SDL_GetWindowSize(window, &window_size_.x, &window_size_.y));

    return true;
}

void Context::create_swap_chain() {
    swap_chain_.init_swap_chain(this);
    swap_chain_.setup_depth_attachment(this);
}
