add_executable(PBR)

file(GLOB_RECURSE SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HED_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

target_sources(PBR PRIVATE ${SRC_FILES} ${HED_FILES})

target_link_libraries(PBR PRIVATE Tipu)