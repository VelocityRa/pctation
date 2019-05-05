add_library(gui STATIC gui.cpp
                       gui.hpp)

# For imgui_memory_editor
target_include_directories(gui PUBLIC ${CMAKE_SOURCE_DIR}/external/imgui_club)

# imgui_memory_editor uses _snprintf
target_compile_definitions(gui PUBLIC _CRT_SECURE_NO_WARNINGS=1)

target_link_libraries(gui PUBLIC SDL2::SDL2
                                 glbinding::glbinding
                                 glbinding::glbinding-aux
                                 imgui::imgui
                                 imgui_sdl2_gl3_backend
                                 io
                                 gpu
                                 emulator
                                 util)
