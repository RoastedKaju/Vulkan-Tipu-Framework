add_library(Common SHARED)

file(GLOB_RECURSE SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HED_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")

target_sources(Common PRIVATE ${SRC_FILES} ${HED_FILES})

target_include_directories(Common PUBLIC "include")

target_link_libraries(Common
        PUBLIC Vulkan::Vulkan
        PUBLIC SDL3::SDL3
        PUBLIC volk
        PUBLIC GPUOpen::VulkanMemoryAllocator
        PUBLIC glm::glm
        PUBLIC TinyGLTF
)

target_compile_definitions(Common PUBLIC VK_NO_PROTOTYPES)
target_compile_definitions(Common PRIVATE GLM_FORCE_RADIANS GLM_FORCE_DEPTH_ZERO_TO_ONE)

# Shader-C linking
target_include_directories(Common PUBLIC "${CMAKE_SOURCE_DIR}/extern/shaderc")
target_link_directories(Common PUBLIC "${CMAKE_SOURCE_DIR}/extern/shaderc/bin")
target_link_libraries(Common PUBLIC shaderc_shared)