#pragma once

#include <volk.h>
#include <SDL3/SDL.h>

#include "attachment.h"

class Context;

class UI {
public:
    UI() = default;

    ~UI();

    void create_imgui_context(Context *context);

    void begin_frame(FrameBuffer &frame_buffer);

    void end_frame() const;

    void shutdown();

private:
    Context *context_;
    VkDescriptorPool desc_pool_{VK_NULL_HANDLE};
    bool imgui_initialized{false};
};
