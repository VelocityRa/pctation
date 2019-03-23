add_library(cpu STATIC cpu.cpp
                       cpu.hpp
                       cpu_instruction.cpp
                       cpu_instruction.hpp
                       cpu_opcode.hpp
                       cpu_opcodes.def)

target_include_directories(cpu PUBLIC .)

target_link_libraries(cpu PUBLIC util bus)
