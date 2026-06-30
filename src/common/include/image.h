#pragma once

#include <vulkan/vulkan.h>

/**
 * Image state is used by memory barriers to save the current state of an image.
 */
struct ImageState {
    VkImageLayout layout{VK_IMAGE_LAYOUT_UNDEFINED};
    VkAccessFlags2 access{0};
    VkPipelineStageFlags2 stage{VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT};
};


/**
 * This is a GPU resource, it is a container for vulkan image handle
 */
struct Image {
    VkImage image = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    ImageState state{};
};
