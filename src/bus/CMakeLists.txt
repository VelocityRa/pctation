add_library(bus STATIC bus.cpp
                       bus.hpp)

target_link_libraries(bus PUBLIC io cpu util bios memory)
