add_library(gui STATIC gui.cpp
                       gui.hpp)

# imgui_memory_editor uses _snprintf
target_compile_definitions(gui PUBLIC _CRT_SECURE_NO_WARNINGS)
target_include_directories(gui PUBLIC ${CMAKE_SOURCE_DIR}/external/imgui_club)
target_link_libraries(gui PUBLIC bgfx gpu imgui emulator util)
