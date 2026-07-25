#include <iostream>
#include <limits>
#include <memory>

#include "utils.h"
#include "context.h"
#include "model.h"
#include "buffer.h"
#include "shader.h"
#include "pipeline.h"

constexpr uint32_t kWidth = 1280u;
constexpr uint32_t kHeight = 720u;
constexpr uint32_t kShadowMapSize = 2048u;

static auto light_direction_ = glm::vec3(15.0f, -100.0f, 10.0f);

struct ShaderData {
    glm::mat4 projection_;
    glm::mat4 view_;
    glm::mat4 light_space_matrix_;
    glm::vec3 light_direction_;
    uint32_t shadow_map_index_;
};

struct alignas(16) PushConstant {
    glm::mat4 model_;
    VkDeviceAddress data_address_;
    uint32_t bindless_albedo_;
};

struct alignas(16) ShadowPushConstant {
    glm::mat4 mvp_;
    uint32_t bindless_albedo_;
};

// The scene's model transform.
glm::mat4 get_mesh_transform() {
    auto transform = glm::mat4(1.0f);
    transform = glm::translate(transform, glm::vec3(0.0f, 0.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    transform = glm::scale(transform, glm::vec3(0.25f, 0.25f, 0.25f));
    return transform;
}

// orthographic frustum.
glm::mat4 compute_light_space_matrix(const glm::vec3 &light_dir, const glm::vec3 &center, const float radius) {
    const glm::vec3 dir = glm::normalize(light_dir);
    const glm::vec3 light_pos = center - dir * radius * 2.0f;
    // Avoid a degenerate lookAt when the light is nearly parallel to world-up.
    const glm::vec3 up = (std::abs(dir.y) > 0.99f) ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);

    const glm::mat4 light_view = glm::lookAt(light_pos, center, up);
    glm::mat4 light_proj = glm::ortho(-radius, radius, -radius, radius, 0.1f, radius * 4.0f);
    light_proj[1][1] *= -1.0f; // Y-flip

    return light_proj * light_view;
}

struct Camera {
    glm::vec3 position_ = glm::vec3(0.0f, 0.0f, 10.0f);
    glm::vec3 front_ = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up_ = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 world_up_ = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right_ = glm::vec3(1.0f, 0.0f, 0.0f);

    float yaw_ = -90.0f;
    float pitch_ = 0.0f;

    float speed_ = 50.0f;
    float sen_ = 0.1f;

    void update() {
        glm::vec3 new_front;
        new_front.x = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
        new_front.y = sin(glm::radians(pitch_));
        new_front.z = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
        front_ = glm::normalize(new_front);
        right_ = glm::normalize(glm::cross(front_, world_up_));
        up_ = glm::normalize(glm::cross(right_, front_));
    }

    void process_keyboard(const bool *keys, const float dt) {
        const float velocity = speed_ * dt;

        if (keys[SDL_SCANCODE_W]) position_ += front_ * velocity;
        if (keys[SDL_SCANCODE_S]) position_ -= front_ * velocity;
        if (keys[SDL_SCANCODE_A]) position_ -= right_ * velocity;
        if (keys[SDL_SCANCODE_D]) position_ += right_ * velocity;
        if (keys[SDL_SCANCODE_SPACE]) position_ += world_up_ * velocity;
        if (keys[SDL_SCANCODE_LCTRL]) position_ -= world_up_ * velocity;
    }

    void process_mouse(const float xrel, const float yrel) {
        yaw_ += xrel * sen_;
        pitch_ -= yrel * sen_;

        if (pitch_ > 89.0f) pitch_ = 89.0f;
        if (pitch_ < -89.0f) pitch_ = -89.0f;

        update();
    }

    glm::mat4 get_view_matrix() const {
        return glm::lookAt(position_, position_ + front_, up_);
    }
};

int main(int argc, char *argv[]) {
    Config config{
        .app_name_ = "Shadows",
        .present_mode_ = VK_PRESENT_MODE_FIFO_KHR,
        .enable_validation_ = true
    };
    const auto ctx = std::make_unique<Context>(config);
    ctx->initialize();
    [[maybe_unused]] SDL_Window *window = ctx->create_window("Shadows Example", kWidth, kHeight);
    SDL_SetWindowRelativeMouseMode(window, true);

    // load model
    Model sponza_model{};
    sponza_model.load(ctx.get(), "assets/models/sponza.glb");

    std::vector<MeshData> sponza_meshes_data;
    std::vector<Buffer> vert_buffers;
    std::vector<Buffer> index_buffers;

    for (auto &i: sponza_model.meshes()) {
        // copy data
        sponza_meshes_data.push_back(i.data());
        // vertices
        const VkDeviceSize v_buf_size = sizeof(Vertex) * sponza_meshes_data.back().vertices_.size();
        BufferDesc v_buf_desc{
            .context = ctx.get(),
            .usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .size = v_buf_size
        };
        vert_buffers.emplace_back();
        vert_buffers.back().create(v_buf_desc);
        vert_buffers.back().update(sponza_meshes_data.back().vertices_.data());
        // indices
        const VkDeviceSize i_buf_size = sizeof(uint32_t) * sponza_meshes_data.back().indices_.size();
        BufferDesc i_buf_desc{
            .context = ctx.get(),
            .usage_flags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            .size = i_buf_size
        };
        index_buffers.emplace_back();
        index_buffers.back().create(i_buf_desc);
        index_buffers.back().update(sponza_meshes_data.back().indices_.data());
    }

    // world-space bounds
    const glm::mat4 mesh_transform = get_mesh_transform();
    glm::vec3 scene_min(std::numeric_limits<float>::max());
    glm::vec3 scene_max(std::numeric_limits<float>::lowest());
    for (const auto &mesh_data: sponza_meshes_data) {
        for (const auto &vertex: mesh_data.vertices_) {
            const glm::vec3 world_pos = glm::vec3(mesh_transform * glm::vec4(vertex.position_, 1.0f));
            scene_min = glm::min(scene_min, world_pos);
            scene_max = glm::max(scene_max, world_pos);
        }
    }
    const glm::vec3 scene_center = (scene_min + scene_max) * 0.5f;
    float scene_radius = glm::length(scene_max - scene_min) * 0.5f * 1.1f;
    scene_radius *= 0.25f;

    // default textures
    std::unique_ptr<Image> white_color = ctx->create_solid_texture(glm::u8vec4(255, 255, 255, 255),
                                                                   VK_FORMAT_R8G8B8A8_SRGB);
    std::unique_ptr<Image> flat_normal = ctx->create_solid_texture(glm::u8vec4(128, 128, 255, 255),
                                                                   VK_FORMAT_R8G8B8A8_UNORM);
    std::unique_ptr<Image> black_tex = ctx->create_solid_texture(glm::u8vec4(0, 50, 125, 255),
                                                                 VK_FORMAT_R8G8B8A8_UNORM);

    // per frame uniform buffer
    BufferDesc u_buf_desc{
        .context = ctx.get(),
        .usage_flags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .size = sizeof(ShaderData),
        .per_frame = true
    };
    Buffer uniform_buffer{};
    uniform_buffer.create(u_buf_desc);

    // load shaders
    const VkShaderModule vert_shader = Shader::create_shader_module(ctx.get(), "assets/shaders/shadows.vert.glsl",
                                                                    shaderc_vertex_shader);
    const VkShaderModule frag_shader = Shader::create_shader_module(ctx.get(), "assets/shaders/shadows.frag.glsl",
                                                                    shaderc_fragment_shader);
    const VkShaderModule shadow_vert_shader = Shader::create_shader_module(
        ctx.get(), "assets/shaders/depth.vert.glsl", shaderc_vertex_shader);
    const VkShaderModule shadow_frag_shader = Shader::create_shader_module(
        ctx.get(), "assets/shaders/depth.frag.glsl", shaderc_fragment_shader);

    // create depth texture
    TextureDesc depth_tex_desc{};
    depth_tex_desc.dimension_ = {kWidth, kHeight};
    depth_tex_desc.mip_levels_ = 1;
    depth_tex_desc.aspect_ = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    depth_tex_desc.array_layers_ = 1;
    depth_tex_desc.depth_ = 1;
    depth_tex_desc.format_ = ctx->get_device_depth_format();
    depth_tex_desc.tiling_ = VK_IMAGE_TILING_OPTIMAL;
    depth_tex_desc.usage_ = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depth_tex_desc.prefer_dedicated_alloc_ = true;
    depth_tex_desc.samples_ = VK_SAMPLE_COUNT_1_BIT;
    auto depth_texture = ctx->create_texture(depth_tex_desc);

    // create shadow texture
    TextureDesc shadow_tex_desc{};
    shadow_tex_desc.dimension_ = {kShadowMapSize, kShadowMapSize};
    shadow_tex_desc.mip_levels_ = 1;
    shadow_tex_desc.aspect_ = VK_IMAGE_ASPECT_DEPTH_BIT;
    shadow_tex_desc.array_layers_ = 1;
    shadow_tex_desc.depth_ = 1;
    shadow_tex_desc.format_ = VK_FORMAT_D32_SFLOAT;
    shadow_tex_desc.tiling_ = VK_IMAGE_TILING_OPTIMAL;
    shadow_tex_desc.usage_ = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    shadow_tex_desc.prefer_dedicated_alloc_ = true;
    shadow_tex_desc.samples_ = VK_SAMPLE_COUNT_1_BIT;
    auto shadow_texture = ctx->create_texture(shadow_tex_desc);

    // pipeline layout
    PipelineLayoutBuilder pipeline_layout_desc{};
    pipeline_layout_desc.add_descriptor_set_layout(ctx->get_texture_registry().get_layout());
    pipeline_layout_desc.add_push_constant(VK_SHADER_STAGE_VERTEX_BIT, sizeof(PushConstant));
    const PipelineLayout pipeline_layout = pipeline_layout_desc.build(ctx.get());

    // Solid pipeline
    PipelineBuilder pipeline_builder{};
    pipeline_builder.add_shader(VK_SHADER_STAGE_VERTEX_BIT, vert_shader);
    pipeline_builder.add_shader(VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader);
    constexpr VkVertexInputBindingDescription vertex_binding{
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    const std::vector<VkVertexInputAttributeDescription> vertex_attributes{
        {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT},
        {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, normal_)},
        {.location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, uv_)},
        {.location = 3, .binding = 0, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(Vertex, tangent_)},
    };
    pipeline_builder.set_vertex_layout(vertex_binding, vertex_attributes);
    pipeline_builder.set_input_assembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipeline_builder.set_viewport(1, 1, true);
    pipeline_builder.set_rasterization(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipeline_builder.set_multisampling(VK_SAMPLE_COUNT_1_BIT);
    pipeline_builder.set_depth_stencil(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
    pipeline_builder.set_color_blend(1, 0xF);
    VkPipeline pipeline = pipeline_builder.build(ctx.get(),
                                                 pipeline_layout,
                                                 {ctx->get_swap_chain().get_format()},
                                                 depth_texture->format_);

    // shadow depth pass
    PipelineLayoutBuilder shadow_pipeline_layout_desc{};
    shadow_pipeline_layout_desc.add_descriptor_set_layout(ctx->get_texture_registry().get_layout());
    shadow_pipeline_layout_desc.add_push_constant(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                                  sizeof(ShadowPushConstant));
    const PipelineLayout shadow_pipeline_layout = shadow_pipeline_layout_desc.build(ctx.get());

    PipelineBuilder shadow_pipeline_builder{};
    shadow_pipeline_builder.add_shader(VK_SHADER_STAGE_VERTEX_BIT, shadow_vert_shader);
    shadow_pipeline_builder.add_shader(VK_SHADER_STAGE_FRAGMENT_BIT, shadow_frag_shader);
    shadow_pipeline_builder.set_vertex_layout(vertex_binding, vertex_attributes);
    shadow_pipeline_builder.set_input_assembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    shadow_pipeline_builder.set_viewport(1, 1, true);
    shadow_pipeline_builder.set_rasterization(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    shadow_pipeline_builder.set_multisampling(VK_SAMPLE_COUNT_1_BIT);
    shadow_pipeline_builder.set_depth_stencil(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
    shadow_pipeline_builder.set_color_blend(0, 0);
    VkPipeline shadow_pipeline = shadow_pipeline_builder.build(ctx.get(),
                                                               shadow_pipeline_layout,
                                                               {}, // no color attachments, depth-only pass
                                                               shadow_texture->format_);

    // loop setup
    uint64_t last_time = SDL_GetPerformanceCounter();
    bool quit = false;

    // camera
    Camera camera{};
    camera.position_ = glm::vec3(0.0f, 3.0f, 10.0f);
    camera.update();

    while (!quit) {
        Uint64 current_time = SDL_GetPerformanceCounter();
        [[maybe_unused]] double delta_time = static_cast<double>(current_time - last_time) /
                                             static_cast<double>(SDL_GetPerformanceFrequency());
        last_time = current_time;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                quit = true;
            }
            if (event.type == SDL_EVENT_MOUSE_MOTION) {
                camera.process_mouse(event.motion.xrel, event.motion.yrel);
            }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
                quit = true;
            }
        }

        const bool *keys = SDL_GetKeyboardState(nullptr);
        camera.process_keyboard(keys, static_cast<float>(delta_time));

        // update shader data
        ShaderData shader_data{};
        shader_data.projection_ = glm::perspective(glm::radians(45.0f),
                                                   1280.0f / 720.0f,
                                                   0.1f, 1000.0f);
        shader_data.projection_[1][1] *= -1.0f; // flip Y
        shader_data.view_ = camera.get_view_matrix();
        shader_data.light_direction_ = glm::normalize(light_direction_);

        const glm::mat4 light_space_matrix = compute_light_space_matrix(light_direction_, scene_center, scene_radius);
        shader_data.light_space_matrix_ = light_space_matrix;
        shader_data.shadow_map_index_ = shadow_texture->bindless_index_;

        [[maybe_unused]] auto time = static_cast<float>(SDL_GetTicks()) / 1000.0f;

        ctx->acquire_command_buffer();
        {
            // shadow depth pass from light's POV
            Attachment shadow_pass{};
            shadow_pass.set_depth(
                VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                VK_ATTACHMENT_LOAD_OP_CLEAR,
                VK_ATTACHMENT_STORE_OP_STORE
            );

            FrameBuffer shadow_frame_buffer{};
            shadow_frame_buffer.depth_image_ = shadow_texture.get();

            ctx->begin_rendering(shadow_pass, shadow_frame_buffer);
            {
                ctx->bind_descriptor_set(shadow_pipeline_layout, ctx->get_texture_registry().get_set());
                ctx->bind_pipeline(shadow_pipeline);
                for (auto i = 0; i < sponza_model.meshes().size(); ++i) {
                    const auto &mat = sponza_model.meshes().at(i).material();

                    ShadowPushConstant spc{};
                    spc.mvp_ = light_space_matrix * mesh_transform;
                    spc.bindless_albedo_ = mat->base_color_
                                               ? mat->base_color_->image_->bindless_index_
                                               : white_color->bindless_index_;

                    ctx->bind_vertex_buffer(vert_buffers.at(i).get());
                    ctx->bind_index_buffer(index_buffers.at(i).get());
                    ctx->cmd_push_constants(shadow_pipeline_layout, &spc);
                    ctx->draw_indexed(sponza_meshes_data.at(i).indices_.size());
                }
            }
            ctx->end_rendering();

            // main scene pass
            Attachment scene_pass{};
            scene_pass.add_color(
                VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                VK_ATTACHMENT_LOAD_OP_CLEAR,
                VK_ATTACHMENT_STORE_OP_DONT_CARE,
                {0.0f, 0.0f, 0.0f, 1.0f}
            );
            scene_pass.set_depth(
                VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                VK_ATTACHMENT_LOAD_OP_CLEAR,
                VK_ATTACHMENT_STORE_OP_DONT_CARE
            );

            FrameBuffer frame_buffer{};
            frame_buffer.depth_image_ = depth_texture.get();
            frame_buffer.color_images_[0] = ctx->get_current_swap_chain_image();

            ctx->begin_rendering(scene_pass, frame_buffer);
            {
                // model
                ctx->bind_descriptor_set(pipeline_layout, ctx->get_texture_registry().get_set());
                ctx->bind_pipeline(pipeline);
                for (auto i = 0; i < sponza_model.meshes().size(); ++i) {
                    PushConstant pc{};
                    pc.data_address_ = uniform_buffer.address();
                    const auto &mat = sponza_model.meshes().at(i).material();

                    pc.model_ = mesh_transform;
                    // color texture
                    pc.bindless_albedo_ = mat->base_color_
                                              ? mat->base_color_->image_->bindless_index_
                                              : white_color->bindless_index_;

                    uniform_buffer.update(&shader_data);

                    ctx->bind_vertex_buffer(vert_buffers.at(i).get());
                    ctx->bind_index_buffer(index_buffers.at(i).get());
                    ctx->cmd_push_constants(pipeline_layout, &pc);
                    ctx->draw_indexed(sponza_meshes_data.at(i).indices_.size());
                }
            }
            ctx->end_rendering();
        }
        ctx->submit();

        // on resize
        if (ctx->get_swap_chain().is_swap_chain_dirty()) {
            ctx->recreate_swap_chain();
            // recreate depth texture
            ctx->destroy_image(depth_texture.get());
            depth_tex_desc.dimension_ = ctx->get_window_size();
            depth_texture = ctx->create_texture(depth_tex_desc);
        }
    }

    // cleanup
    // waits for device to be idle
    ctx->wait_idle();
    // clean up resources
    for (auto i = 0; i < sponza_model.meshes().size(); ++i) {
        vert_buffers.at(i).destroy();
        index_buffers.at(i).destroy();
    }
    uniform_buffer.destroy();
    ctx->destroy_pipeline_layout(pipeline_layout);
    ctx->destroy_pipeline(pipeline);
    ctx->destory_shader(vert_shader);
    ctx->destory_shader(frag_shader);
    ctx->destroy_pipeline_layout(shadow_pipeline_layout);
    ctx->destroy_pipeline(shadow_pipeline);
    ctx->destory_shader(shadow_vert_shader);
    ctx->destory_shader(shadow_frag_shader);
    ctx->destroy_image(shadow_texture.get());
    ctx->destroy_image(depth_texture.get());
    ctx->destroy_image(white_color.get());
    ctx->destroy_image(flat_normal.get());
    ctx->destroy_image(black_tex.get());
    sponza_model.destroy_textures();
    // destroy window, instance and device
    ctx->destroy();
}
