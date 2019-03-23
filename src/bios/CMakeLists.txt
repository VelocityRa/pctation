add_library(bios STATIC bios.cpp
                        bios.hpp)

target_link_libraries(bios PUBLIC util)
