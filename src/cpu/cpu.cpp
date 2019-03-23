#include <bus/bus.hpp>
#include <cpu/cpu.hpp>
#include <cpu/instruction.hpp>
#include <cpu/opcode.hpp>
#include <util/log.hpp>

namespace cpu {

Cpu::Cpu(bus::Bus const& bus) : m_bus(bus) {}

bool Cpu::step(u32& cycles_passed) {
  // Fetch instruction
  const u32 instr_value = m_bus.read32(m_pc);

  // Increment Program Counter
  m_pc += 4;

  // Decode instruction
  const Instruction instr(instr_value);

  LOG::debug("0x{:08X}: {}", instr_value, opcode_to_str(instr.opcode()));

  // TODO: Disassembler (we could add an extra field in opcodes.def

  // Execute instruction

  execute_instruction(instr);


  cycles_passed = 0;  // TODO
  return false;
}

void Cpu::execute_instruction(const Instruction& i) {
  switch (i.opcode()) {
    case Opcode::LUI: r(i.rt()) = i.imm16() << 16; break;
    case Opcode::ORI: ;
    default:
      LOG::warn("Unimplemented instruction executed: 0x{:08X}",
                static_cast<u32>(i.opcode()));
  }
}

}  // namespace cpu
