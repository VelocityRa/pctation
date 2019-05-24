add_library(cpu STATIC cpu.cpp
                       cpu.hpp
                       instruction.cpp
                       instruction.hpp
                       opcode.hpp
                       opcodes.def
                       interrupt.cpp
                       interrupt.hpp
                       gte.cpp
                       gte.hpp)

target_link_libraries(cpu PUBLIC emulator util bus)
