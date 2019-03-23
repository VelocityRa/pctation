add_library(memory STATIC range.cpp
                          range.hpp
                          map.hpp)

target_link_libraries(memory PUBLIC util)
