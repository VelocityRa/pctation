add_library(bios STATIC bios.cpp
                        bios.hpp)

target_include_directories(bios PUBLIC .)

target_link_libraries(bios PUBLIC util)
