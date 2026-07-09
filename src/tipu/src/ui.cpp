#include "ui.h"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include "utils.h"
#include "context.h"
#include "swap_chain.h"

UI::~UI() {
}

void UI::create_imgui_context(Context *context) {
    context_ = context;
    VkDevice device = context->get_device();
    SDL_Window *window = context->get_window();
    SwapChain &swap_chain = context->get_swap_chain();

    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
    };

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * std::size(pool_sizes);
    pool_info.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes));
    pool_info.pPoolSizes = pool_sizes;

    check(vkCreateDescriptorPool(device, &pool_info, nullptr, &desc_pool_));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsClassic();
    ImGui_ImplSDL3_InitForVulkan(window);

    // clamp the min image count
    uint32_t min_img_count = swap_chain.get_surface_capabilities().minImageCount + 1;

    if (swap_chain.get_surface_capabilities().maxImageCount > 0) {
        min_img_count = std::min(min_img_count, swap_chain.get_surface_capabilities().maxImageCount);
    }

    VkFormat color_format = swap_chain.get_format();

    VkPipelineRenderingCreateInfo rendering_info{};
    rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachmentFormats = &color_format;

    ImGui_ImplVulkan_PipelineInfo pipeline_info{};
    pipeline_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    pipeline_info.PipelineRenderingCreateInfo = rendering_info;

    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance = context->get_instance();
    init_info.PhysicalDevice = context->get_physical_device();
    init_info.Device = context->get_device();
    init_info.QueueFamily = context->get_family_index();
    init_info.Queue = context->get_queue();
    init_info.DescriptorPool = desc_pool_;
    init_info.MinImageCount = min_img_count;
    init_info.ImageCount = static_cast<uint32_t>(swap_chain.get_images().size());
    init_info.PipelineInfoMain = pipeline_info;
    init_info.UseDynamicRendering = true;
    init_info.ApiVersion = VK_API_VERSION_1_3;

    ImGui_ImplVulkan_Init(&init_info);

    imgui_initialized = true;

    std::cout << "ImGUI context created.\n";
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void UI::begin_frame(FrameBuffer &frame_buffer) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void UI::end_frame() const {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), context_->get_current_cmd_buf());
}

void UI::shutdown() {
    if (!imgui_initialized) {
        return;
    }

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    vkDestroyDescriptorPool(context_->get_device(), desc_pool_, nullptr);
    desc_pool_ = VK_NULL_HANDLE;
    imgui_initialized = false;
}
