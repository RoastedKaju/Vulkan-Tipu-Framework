#pragma once

#include <iostream>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vma/vk_mem_alloc.h>

#include "swap_chain.h"

class Context {
public:
    explicit Context();

    explicit Context(bool enable_validation_layers);

    ~Context();

    bool create_instance(const char *app_name = "default");

    bool setup_device(uint32_t device_index = 0);

    bool create_window(const char *title, uint32_t width, uint32_t height);

    void create_swap_chain();

private:
    bool enable_validation_layers_{true};
    const std::vector<const char *> validation_layers_{"VK_LAYER_KHRONOS_validation"};
    VkDebugUtilsMessengerEXT debug_messenger_{VK_NULL_HANDLE};

    // window
    SDL_Window *window_{nullptr};
    glm::ivec2 window_size_{};

    // device
    VkInstance instance_{VK_NULL_HANDLE};
    VkPhysicalDevice physical_device_{VK_NULL_HANDLE};
    VkDevice device_{VK_NULL_HANDLE};
    VkQueue queue_{VK_NULL_HANDLE};
    VkSurfaceKHR surface_{VK_NULL_HANDLE};
    uint32_t device_index_{0};

    // allocator
    VmaAllocator allocator_{VK_NULL_HANDLE};

    // swap-chain
    SwapChain swap_chain_;

    friend class SwapChain;
};
