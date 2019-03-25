#include <bus/bus.hpp>
#include <cpu/cpu.hpp>
#include <cpu/instruction.hpp>
#include <cpu/opcode.hpp>
#include <util/log.hpp>

#include <limits>

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
  LOG_TRACE("[{:08X}]: {:08X} {}", m_previous_pc, cur_instr, instr.disassemble());

  m_previous_pc = m_pc;

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
    case Opcode::ADD: set_rd(i, checked_add(rs(i), rt(i))); break;
    case Opcode::ADDU: set_rd(i, rs(i) + rt(i)); break;
    case Opcode::SUB: set_rd(i, checked_sub(rs(i), rt(i))); break;
    case Opcode::SUBU: set_rd(i, rs(i) - rt(i)); break;
    case Opcode::ADDI: set_rt(i, checked_add(rs(i), i.imm16_se())); break;
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
    // Memory operations (loads)
    case Opcode::LW: issue_pending_load(i.rt(), read32(i.imm16_se() + rs(i))); break;
    // Memory operations (stores)
    case Opcode::SW: write32(i.imm16_se() + rs(i), rt(i)); break;
    // Jumps/Branches
    case Opcode::J: pc() = pc() & 0xF0000000 | (i.imm26() << 2); break;
    case Opcode::BNE:
      if (rs(i) != rt(i))
        branch(i.imm16_se());
      break;
    // Co-processor 0
    case Opcode::MTC0: {
      const auto cop_dst_reg = static_cast<Cop0Register>(i.rd());

      switch (cop_dst_reg) {
        case Cop0Register::COP0_BPC: break;
        case Cop0Register::COP0_BDA: break;
        case Cop0Register::COP0_JUMPDEST: break;
        case Cop0Register::COP0_DCIC: break;
        case Cop0Register::COP0_BAD_VADDR: break;
        case Cop0Register::COP0_BDAM: break;
        case Cop0Register::COP0_BPCM: break;
        case Cop0Register::COP0_SR: m_sr = rt(i); break;
        case Cop0Register::COP0_CAUSE: break;
        case Cop0Register::COP0_EPC: break;
        case Cop0Register::COP0_PRID: break;
        default: LOG_WARN("Unhandled Cop1 register 0x{:08X}", cop_dst_reg); assert(0);
      }
      break;
    }

    default: LOG_ERROR("Unimplemented instruction executed"); assert(0);
  }

  check_pending_load();
}

void Cpu::branch(s16 offset) {
  pc() += (offset << 2) - 4;
}

u32 Cpu::checked_add(s32 op1, s32 op2) {
  //  if (op1 > 0 && op2 > 0 && op1 + op2 < 0)
  if (op1 > 0 && op2 > 0 && op1 > INT_MAX - op2 || op1 < 0 && op2 < 0 && op1 < INT_MIN - op2)
    assert(0);
  return op1 + op2;
}

u32 Cpu::checked_sub(s32 op1, s32 op2) {
  if (op1 < 0 && op2 > INT_MAX + op1 || op1 > 0 && op2 < INT_MIN + op1)
    assert(0);
  return op1 - op2;
}

// u32 Cpu::checked_mul(u32 op1, u32 op2) {
//  if (static_cast<u64>(op1) * static_cast<u64>(op2) > 0x100000000LL)
//    assert(0);  // TODO: overflow trap
//  return op1 - op2;
//}

u32 Cpu::read32(u32 addr) const {
  return m_bus.read32(addr);
}

void Cpu::write32(u32 addr, u32 val) {
  if (m_sr & COP0_SR_ISOLATE_CACHE) {
    LOG_DEBUG("Ignoring write 0x{:08X} to 0x{:08X} due to cache isolation", val, addr);
    return;
  }
  m_bus.write32(addr, val);
}

}  // namespace cpu
