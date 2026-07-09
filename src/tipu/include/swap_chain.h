#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "image.h"

class Context;

class SwapChain {
public:
    SwapChain() = default;

    SwapChain(const SwapChain &) = delete;

    SwapChain(SwapChain &&) = delete;

    SwapChain &operator=(const SwapChain &) = delete;

    SwapChain &operator=(SwapChain &&) = delete;

    ~SwapChain() = default;

    void init_swap_chain(const Context *context, VkFormat image_format = VK_FORMAT_R8G8B8A8_SRGB);

    void recreate_swap_chain(Context *context);

    VkSwapchainKHR get() const { return swap_chain_; }

    VkFormat get_format() const { return swap_chain_format_; };
    std::vector<Image> &get_images() { return swap_chain_images_; }

    void mark_swap_chain_dirty() { is_swap_chain_dirty_ = true; }
    bool is_swap_chain_dirty() const { return is_swap_chain_dirty_; }
    VkSurfaceCapabilitiesKHR get_surface_capabilities() const { return surface_caps_; }

private:
    bool is_swap_chain_dirty_{false};
    VkSwapchainKHR swap_chain_{VK_NULL_HANDLE};
    VkFormat swap_chain_format_{VK_FORMAT_R8G8B8A8_SRGB};
    std::vector<Image> swap_chain_images_;
    VkSurfaceCapabilitiesKHR surface_caps_;
};
