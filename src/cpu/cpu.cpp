#include <bus.hpp>
#include <cpu.hpp>
#include <cpu_instruction.hpp>
#include <cpu_opcode.hpp>
#include <log.hpp>

namespace cpu {

Cpu::Cpu(bus::Bus const& bus) : m_bus(bus) {}

bool Cpu::step(u32& cycles_passed) {
  /* Fetch instruction */
  const u32 instr_value = m_bus.read32(m_pc);

  /* Increment Program Counter */
  m_pc += 4;

  /* Decode instruction */
  const Instruction instr(instr_value);

  LOG::debug("0x{:08X}: {}", instr_value, opcode_to_str(instr.opcode()));

  /* Execute instruction */
  // TODO:

  cycles_passed = 0; // TODO
  return false;
}

}  // namespace cpu
