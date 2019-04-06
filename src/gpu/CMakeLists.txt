add_library(gpu STATIC gpu.cpp
                       gpu.hpp)

target_link_libraries(gpu PUBLIC util)
