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
    // Comparison
    case Opcode::SLT: set_rd(i, (s32)rs(i) < (s32)rt(i) ? 1 : 0); break;
    case Opcode::SLTU: set_rd(i, rs(i) < rt(i) ? 1 : 0); break;
    case Opcode::SLTI: set_rt(i, (s32)rs(i) < i.imm16_se() ? 1 : 0); break;
    case Opcode::SLTIU:
      set_rt(i, rs(i) < (u32)i.imm16_se() ? 1 : 0);
      break;  // TODO: verify
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
    case Opcode::LB: issue_pending_load(i.rt(), (s8)read8(i.imm16_se() + rs(i))); break;
    case Opcode::LW:
      issue_pending_load(i.rt(), read32(i.imm16_se() + rs(i)));
      break;
      // Memory operations (stores)
    case Opcode::SB: write8(i.imm16_se() + rs(i), (u8)rt(i)); break;
    case Opcode::SH: write16(i.imm16_se() + rs(i), (u16)rt(i)); break;
    case Opcode::SW: write32(i.imm16_se() + rs(i), rt(i)); break;
    // Jumps/Branches
    case Opcode::J: op_jump(i); break;
    case Opcode::JR: pc() = rs(i); break;
    case Opcode::JAL:
      r(31) = pc();
      op_jump(i);
      break;
    case Opcode::BEQ:
      if (rs(i) == rt(i))
        op_branch(i);
      break;
    case Opcode::BNE:
      if (rs(i) != rt(i))
        op_branch(i);
      break;
    // Co-processor 0
    case Opcode::MTC0: {
      const auto cop_dst_reg = static_cast<Cop0Register>(i.rd());
      switch (cop_dst_reg) {
        case Cop0Register::COP0_BPC:
        case Cop0Register::COP0_BDA:
        case Cop0Register::COP0_JUMPDEST:
        case Cop0Register::COP0_DCIC:
        case Cop0Register::COP0_BAD_VADDR:
        case Cop0Register::COP0_BDAM:
        case Cop0Register::COP0_BPCM:
        case Cop0Register::COP0_CAUSE:
        case Cop0Register::COP0_EPC:
        case Cop0Register::COP0_PRID: break;
        case Cop0Register::COP0_SR: m_sr = rt(i); break;
        default: LOG_WARN("Unhandled Cop1 register 0x{:08X}", static_cast<u32>(cop_dst_reg)); assert(0);
      }
      break;
    }

    default: LOG_ERROR("Unimplemented instruction executed"); assert(0);
  }

  check_pending_load();
}

void Cpu::op_jump(const Instruction& i) {
  pc() = pc() & 0xF0000000 | (i.imm26() << 2);
}

void Cpu::op_branch(const Instruction& i) {
  pc() += (i.imm16_se() << 2) - 4;
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

u8 Cpu::read8(u32 addr) const {
  return m_bus.read8(addr);
}

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
void Cpu::write16(u32 addr, u16 val) {
  if (m_sr & COP0_SR_ISOLATE_CACHE) {
    LOG_DEBUG("Ignoring write 0x{:04X} to 0x{:08X} due to cache isolation", val, addr);
    return;
  }
  m_bus.write16(addr, val);
}

void Cpu::write8(u32 addr, u8 val) {
  if (m_sr & COP0_SR_ISOLATE_CACHE) {
    LOG_DEBUG("Ignoring write 0x{:02X} to 0x{:08X} due to cache isolation", val, addr);
    return;
  }
  m_bus.write8(addr, val);
}

}  // namespace cpu
