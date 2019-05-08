add_library(renderer STATIC renderer.cpp
                            renderer.hpp
                            shader.cpp
                            shader.hpp
                            buffer.hpp
                            shaders/screen.fs.glsl
                            shaders/screen.vs.glsl)

target_link_libraries(renderer PUBLIC util)
target_link_libraries(renderer PRIVATE SDL2::SDL2 glbinding::glbinding imgui::imgui gpu glm)

add_custom_command(
    TARGET renderer
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders"
)
