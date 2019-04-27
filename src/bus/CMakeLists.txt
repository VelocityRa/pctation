add_library(bus STATIC bus.cpp
                       bus.hpp)

target_link_libraries(bus PUBLIC cpu util bios memory)
