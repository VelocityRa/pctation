add_library(memory STATIC addressable.hpp
                          range.cpp
                          range.hpp
                          ram.cpp
                          ram.hpp
                          dma.cpp
                          dma.hpp
                          dma_channel.cpp
                          dma_channel.hpp
                          map.hpp
                          expansion.cpp
                          expansion.hpp)

target_link_libraries(memory PUBLIC util)
