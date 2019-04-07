add_library(gui STATIC gui.cpp
                       gui.hpp)

target_include_directories(gui PUBLIC ${CMAKE_SOURCE_DIR}/external/imgui_club)
target_link_libraries(gui PUBLIC bgfx imgui emulator util)
