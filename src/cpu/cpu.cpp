#include <bus/bus.hpp>
#include <cpu/cpu.hpp>
#include <cpu/instruction.hpp>
#include <cpu/opcode.hpp>
#include <util/log.hpp>

namespace cpu {

Cpu::Cpu(bus::Bus& bus) : m_bus(bus) {}

bool Cpu::step(u32& cycles_passed) {
  // Current instruction is the previously 'next' one
  u32 cur_instr = m_next_instr;

  // Fetch next instruction, save for next execution step
  m_next_instr = m_bus.read32(m_pc);

  // Decode current instruction
  const Instruction instr(cur_instr);

  // Log instruction disassembly
  LOG_TRACE("[{:08X}]: {:08X} {}", m_pc, cur_instr, instr.disassemble());

  // Increment Program Counter
  m_pc += 4;

  // Execute instruction
  execute_instruction(instr);

  cycles_passed = 0;  // TODO
  return false;
}

void Cpu::execute_instruction(const Instruction& i) {
  switch (i.opcode()) {
      // Arithmetic
    case Opcode::ADD: set_rd(i, rs(i) + rt(i)); break;  // TODO: overflow trap
    case Opcode::ADDU: set_rd(i, rs(i) + rt(i)); break;
    case Opcode::SUB: set_rd(i, rs(i) - rt(i)); break;  // TODO: overflow trap
    case Opcode::SUBU: set_rd(i, rs(i) - rt(i)); break;
    case Opcode::ADDI: set_rt(i, rs(i) + i.imm16_se()); break;  // TODO: overflow trap
    case Opcode::ADDIU: set_rt(i, rs(i) + i.imm16_se()); break;
    // Shifting
    case Opcode::LUI: set_rt(i, i.imm16() << 16); break;
    case Opcode::SLLV: set_rd(i, rt(i) << (rs(i) & 0x1F)); break;
    case Opcode::SRLV: set_rd(i, rt(i) >> (rs(i) & 0x1F)); break;
    case Opcode::SRAV: set_rd(i, (s32)rt(i) >> (rs(i) & 0x1F)); break;  // TODO: verify
    case Opcode::SLL: set_rd(i, rt(i) << i.imm5()); break;
    case Opcode::SRL: set_rd(i, rt(i) >> i.imm5()); break;
    case Opcode::SRA:
      set_rd(i, (s32)rt(i) >> i.imm5());
      break;  // TODO: verify
    // Logical
    case Opcode::ANDI: set_rt(i, rs(i) & i.imm16()); break;
    case Opcode::ORI: set_rt(i, rs(i) | i.imm16()); break;
    case Opcode::XORI: set_rt(i, rs(i) ^ i.imm16()); break;
    case Opcode::AND: set_rd(i, rs(i) & rt(i)); break;
    case Opcode::OR: set_rd(i, rs(i) | rt(i)); break;
    case Opcode::XOR: set_rd(i, rs(i) ^ rt(i)); break;
    case Opcode::NOR: set_rd(i, 0xFFFFFFFF ^ (rs(i) | rt(i))); break;
    // Store
    case Opcode::SW: write32(i.imm16_se() + rs(i), rt(i)); break;
    // Jump
    case Opcode::J: pc() = pc() & 0xF0000000 | (i.imm26() << 2); break;

    default: LOG_ERROR("Unimplemented instruction executed"); assert(0);
  }
}

u32 Cpu::read32(u32 addr) const {
  return m_bus.read32(addr);
}

void Cpu::write32(u32 addr, u32 val) {
  m_bus.write32(addr, val);
}

}  // namespace cpu
