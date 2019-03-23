add_library(emulator STATIC emulator.cpp
                            emulator.hpp)

target_link_libraries(emulator PUBLIC bus cpu util bios)
