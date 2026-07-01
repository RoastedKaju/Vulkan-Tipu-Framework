#include "attachment.h"

#include <cassert>

uint32_t Attachment::add_color(const VkImageLayout layout,
                               const VkAttachmentLoadOp load_op,
                               const VkAttachmentStoreOp store_op,
                               const VkClearColorValue clear) {
    assert(color_count_ < kMaxColorAttachments && "exceeded max color attachments");
    colors_[color_count_] = VkRenderingAttachmentInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageLayout = layout,
        .loadOp = load_op,
        .storeOp = store_op,
        .clearValue = {.color = clear}
    };
    return color_count_++;
}

void Attachment::set_depth(const VkImageLayout layout,
                           const VkAttachmentLoadOp load_op,
                           const VkAttachmentStoreOp store_op,
                           const VkClearDepthStencilValue clear) {
    depth_ = VkRenderingAttachmentInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageLayout = layout,
        .loadOp = load_op,
        .storeOp = store_op,
        .clearValue = {.depthStencil = clear}
    };
    has_depth_ = true;
}
