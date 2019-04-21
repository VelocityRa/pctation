add_library(spu STATIC spu.cpp
                       spu.hpp)

target_link_libraries(spu PUBLIC util)
