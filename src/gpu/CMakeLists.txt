add_library(gpu STATIC gpu.cpp
                       gpu.hpp
                       colors.hpp)

target_link_libraries(gpu PUBLIC renderer util)
