add_library(emulator STATIC emulator.cpp
                            emulator.hpp)

target_include_directories(emulator PUBLIC .)

target_link_libraries(emulator PUBLIC bus cpu util bios)
