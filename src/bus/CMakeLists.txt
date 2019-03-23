add_library(bus STATIC bus.cpp
                       bus.hpp)

target_include_directories(bus PUBLIC .)

target_link_libraries(bus PUBLIC util bios)
