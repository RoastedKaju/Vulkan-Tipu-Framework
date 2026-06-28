#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

class Context;

class Buffer {
public:
    Buffer() = default;

    void create_buffer(const Context *context, VkDeviceSize size, VkBufferUsageFlags usage);

    void copy_data(const void *src) const;

    VkDeviceAddress device_address(const Context *context) const;

private:
    VkBuffer buffer_{VK_NULL_HANDLE};
    VmaAllocation allocation_{VK_NULL_HANDLE};
    VmaAllocationInfo allocation_info_{};
    VkDeviceSize size_{0};
};
