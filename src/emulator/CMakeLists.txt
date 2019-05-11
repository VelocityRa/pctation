add_library(emulator STATIC emulator.cpp
                            emulator.hpp
                            settings.hpp)

target_link_libraries(emulator PUBLIC bus cpu util bios gpu spu)
