add_library(cpu STATIC cpu.cpp
                       cpu.hpp
                       instruction.cpp
                       instruction.hpp
                       opcode.hpp
                       opcodes.def
                       interrupt.hpp
                       interrupt.cpp)

target_link_libraries(cpu PUBLIC util bus)
