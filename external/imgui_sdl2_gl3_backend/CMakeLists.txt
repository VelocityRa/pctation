add_library(imgui_sdl2_gl3_backend STATIC imgui_impl_opengl3.cpp
                                          imgui_impl_opengl3.h
                                          imgui_impl_sdl.cpp
                                          imgui_impl_sdl.h)

target_compile_definitions(imgui_sdl2_gl3_backend PUBLIC IMGUI_IMPL_OPENGL_LOADER_CUSTOM=<glbinding/gl/gl.h>)

target_link_libraries(imgui_sdl2_gl3_backend PUBLIC SDL2::SDL2 glbinding::glbinding)
