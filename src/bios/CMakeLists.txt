add_library(bios STATIC bios.cpp
                        bios.hpp
                        functions.hpp)

target_link_libraries(bios PUBLIC cpu util)
