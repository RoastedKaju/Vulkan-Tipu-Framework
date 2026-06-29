#include "pipeline.h"

#include <volk.h>

#include "utils.h"

PipelineLayoutBuilder &PipelineLayoutBuilder::add_descriptor_set_layout(const VkDescriptorSetLayout layout) {
    set_layouts_.push_back(layout);
    return *this;
}

PipelineLayoutBuilder &PipelineLayoutBuilder::add_push_constant(const VkShaderStageFlags stage,
                                                                const uint32_t size,
                                                                const uint32_t offset) {
    const VkPushConstantRange range{stage, offset, size};
    push_constants_.push_back(range);
    return *this;
}

VkPipelineLayout PipelineLayoutBuilder::build(const Context *context, const std::string &debug_name) {
    const VkPipelineLayoutCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(set_layouts_.size()),
        .pSetLayouts = set_layouts_.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(push_constants_.size()),
        .pPushConstantRanges = push_constants_.data()
    };

    VkPipelineLayout layout;
    check(vkCreatePipelineLayout(context->device_, &info, nullptr, &layout));
    std::printf("Created pipeline layout: %s.\n", debug_name.c_str());
    return layout;
}

PipelineBuilder &PipelineBuilder::add_shader(const VkShaderStageFlagBits stage,
                                             const VkShaderModule module,
                                             const char *entry) {
    const VkPipelineShaderStageCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = stage,
        .module = module,
        .pName = "main"
    };
    shader_stages_.push_back(info);
    return *this;
}

PipelineBuilder &PipelineBuilder::set_vertex_Layout(
    const VkVertexInputBindingDescription &binding,
    const std::vector<VkVertexInputAttributeDescription> &attributes) {
    vertex_input_state_ = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &binding,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size()),
        .pVertexAttributeDescriptions = attributes.data()
    };
    return *this;
}

PipelineBuilder &PipelineBuilder::set_input_assembly(const VkPrimitiveTopology topology) {
    input_assembly_state_ = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = topology
    };
    return *this;
}

PipelineBuilder &PipelineBuilder::set_viewport(const uint32_t viewport_count,
                                               const uint32_t scissor_count,
                                               const bool dynamic) {
    viewport_state_ = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = viewport_count,
        .scissorCount = scissor_count,
    };

    if (dynamic) {
        dynamic_states_.push_back(VK_DYNAMIC_STATE_VIEWPORT);
        dynamic_states_.push_back(VK_DYNAMIC_STATE_SCISSOR);
    }
    return *this;
}

PipelineBuilder &PipelineBuilder::set_rasterization(const VkCullModeFlags cull, const VkFrontFace front) {
    rasterization_state_ = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .cullMode = cull,
        .frontFace = front,
        .lineWidth = 1.0f
    };
    return *this;
}

PipelineBuilder &PipelineBuilder::set_multisampling(const VkSampleCountFlagBits samples) {
    multisample_state_ = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = samples
    };
    return *this;
}

PipelineBuilder &PipelineBuilder::set_depth_stencil(const bool depth_test,
                                                    const bool depth_write,
                                                    const VkCompareOp compare_op) {
    depth_stencil_state_ = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = depth_test ? VK_TRUE : VK_FALSE,
        .depthWriteEnable = depth_write ? VK_TRUE : VK_FALSE,
        .depthCompareOp = compare_op
    };
    return *this;
}

PipelineBuilder &PipelineBuilder::set_color_blend(const VkColorComponentFlags mask) {
    blend_attachment_ = {.colorWriteMask = mask};
    color_blend_state_ = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &blend_attachment_
    };
    return *this;
}

VkPipeline PipelineBuilder::build(const Context *context,
                                  const VkPipelineLayout layout,
                                  VkFormat color_format,
                                  const VkFormat depth_format,
                                  const std::string &debug_name) {
    VkPipelineRenderingCreateInfo rendering_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &color_format,
        .depthAttachmentFormat = depth_format
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamic_states_.size()),
        .pDynamicStates = dynamic_states_.data()
    };

    const VkGraphicsPipelineCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &rendering_info,
        .stageCount = static_cast<uint32_t>(shader_stages_.size()),
        .pStages = shader_stages_.data(),
        .pVertexInputState = &vertex_input_state_,
        .pInputAssemblyState = &input_assembly_state_,
        .pViewportState = &viewport_state_,
        .pRasterizationState = &rasterization_state_,
        .pMultisampleState = &multisample_state_,
        .pDepthStencilState = &depth_stencil_state_,
        .pColorBlendState = &color_blend_state_,
        .pDynamicState = &dynamic_state_info,
        .layout = layout
    };

    VkPipeline pipeline;
    check(vkCreateGraphicsPipelines(context->device_, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline));
    std::printf("Created pipeline: %s.\n", debug_name.c_str());
    return pipeline;
}
