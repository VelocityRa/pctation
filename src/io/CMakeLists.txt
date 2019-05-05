add_library(io STATIC joypad.cpp
                      joypad.hpp
                      digital_controller.cpp
                      digital_controller.hpp)

target_link_libraries(io PUBLIC cpu util)
target_link_libraries(io PRIVATE SDL2::SDL2)
