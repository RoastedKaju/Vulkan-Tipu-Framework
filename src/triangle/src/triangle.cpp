#include <iostream>
#include <memory>

#include "context.h"
#include "swap_chain.h"
#include "buffer.h"
#include "utils.h"
#include "shader.h"
#include "pipeline.h"
#include "texture_descriptor_set.h"

int main(int argc, char *argv[])
{
    Config config{
        .app_name_ = "Triangle Example",
        .present_mode_ = VK_PRESENT_MODE_FIFO_KHR,
        .enable_validation_ = true};
    std::unique_ptr<Context> ctx = std::make_unique<Context>(config);
    if (!ctx->initialize())
    {
        std::cout << "Failed to init context.\n";
        return EXIT_FAILURE;
    }

    SDL_Window *window = ctx->create_window("Triangle", 800u, 600u);

    // shaders
    const VkShaderModule vert_shader = Shader::create_shader_module(ctx.get(), "assets/shaders/triangle_vert.glsl", shaderc_vertex_shader);
    const VkShaderModule frag_shader = Shader::create_shader_module(ctx.get(), "assets/shaders/triangle_frag.glsl", shaderc_fragment_shader);

    // texture descriptor set
    TextureDescriptorSet descriptor_set{};
    descriptor_set.create(ctx->get_device(), 64);

    // create pipeline layout
    PipelineLayoutBuilder pipeline_layout_desc{};
    pipeline_layout_desc.add_descriptor_set_layout(descriptor_set.layout());
    pipeline_layout_desc.add_push_constant(VK_SHADER_STAGE_VERTEX_BIT, sizeof(VkDeviceAddress));
    const VkPipelineLayout pipeline_layout = pipeline_layout_desc.build(ctx.get());

    // create pipeline
    PipelineBuilder pipeline_builder{};
    pipeline_builder.add_shader(VK_SHADER_STAGE_VERTEX_BIT, vert_shader);
    pipeline_builder.add_shader(VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader);
    pipeline_builder.set_input_assembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipeline_builder.set_viewport(1, 1, true);
    pipeline_builder.set_rasterization(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    pipeline_builder.set_multisampling(VK_SAMPLE_COUNT_1_BIT);
    pipeline_builder.set_color_blend(1, 0xF);
    VkPipeline pipeline = pipeline_builder.build(ctx.get(),
                                                 pipeline_layout,
                                                 {ctx->get_swap_chain().get_format()},
                                                 VK_FORMAT_UNDEFINED);

    bool quit = false;

    while (!quit)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                quit = true;
            }
        }

        ctx->acquire_command_buffer();
        {
            // attachments
            Attachment scene_pass{};
            scene_pass.add_color(
                VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                VK_ATTACHMENT_LOAD_OP_CLEAR,
                VK_ATTACHMENT_STORE_OP_STORE,
                {0.0f, 0.0f, 0.0f, 1.0f});

            FrameBuffer frame_buffer{};

            ctx->begin_rendering(scene_pass, frame_buffer);
            {
                ctx->bind_pipeline(pipeline);
                ctx->bind_descriptor_set(pipeline_layout, descriptor_set.get());
                ctx->draw(3);
            }
            ctx->end_rendering();
        }
        ctx->submit();
    }

    return EXIT_SUCCESS;
}