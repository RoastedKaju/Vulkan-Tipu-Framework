add_library(ImGUI STATIC
        imgui.cpp
        imgui_draw.cpp
        imgui_tables.cpp
        imgui_widgets.cpp
        imgui_demo.cpp
        backends/imgui_impl_sdl3.cpp
        backends/imgui_impl_sdlrenderer3.cpp
        backends/imgui_impl_vulkan.cpp
        misc/cpp/imgui_stdlib.cpp
)

target_include_directories(ImGUI PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/backends
        ${CMAKE_CURRENT_SOURCE_DIR}/misc/cpp
)

target_link_libraries(ImGUI PRIVATE SDL3::SDL3 volk)

target_compile_definitions(ImGUI PUBLIC IMGUI_IMPL_VULKAN_USE_VOLK)
