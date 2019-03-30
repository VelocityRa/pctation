#include <bus/bus.hpp>
#include <cpu/cpu.hpp>
#include <cpu/instruction.hpp>
#include <cpu/opcode.hpp>
#include <util/log.hpp>

#include <iostream>
#include <limits>
#include <string>

#define TRACE_NONE 0
#define TRACE_INST 1
#define TRACE_REGS 2
#define TRACE_PC_ONLY 3

#define TRACE_MODE TRACE_INST

#define TTY_OUTPUT 1

namespace cpu {

Cpu::Cpu(bus::Bus& bus) : m_bus(bus) {}

bool Cpu::step(u32& cycles_passed) {
  // Current instruction is the previously 'next' one
  u32 cur_instr = m_next_instr;

  // Fetch next instruction, save for next execution step
  m_next_instr = m_bus.read32(m_pc);

  // Decode current instruction
  const Instruction instr(cur_instr);  // todo: find out the strings that fail strcmp

  // Log instruction disassembly

#if TRACE_MODE == TRACE_REGS
  std::string debug_str;
  for (auto i = 1; i < 32; ++i)
    debug_str +=
        fmt::format("{}:{:X} ", reinterpret_cast<const char*>(register_to_str(i)) + 1, m_gpr[i]);
  debug_str += fmt::format("hi:{:X} lo:{:X}", m_hi, m_lo);
  LOG_TRACE("[{:08X}]: {:08X} {}\n  {}", m_previous_pc, cur_instr, instr.disassemble(), debug_str);
#elif TRACE_MODE == TRACE_INST
  LOG_TRACE("[{:08X}]: {:08X} {}", m_previous_pc, cur_instr, instr.disassemble());
#endif
  m_previous_pc = m_pc;

  // Increment Program Counter
  m_pc += 4;

#if TTY_OUTPUT
  // TODO: Check in JAL(?)
  if (m_pc == 0x4074) {
    char tty_out_char = r(4);
    std::cout << tty_out_char;
  }
#endif

  // Execute instruction
  execute_instruction(instr);

  // In PC_ONLY mode we print the PC-4 for branch delay slot instructions (to match no$psx's output)
#if TRACE_MODE == TRACE_PC_ONLY
  LOG_TRACE("{:08X}", m_pc - 4);
#endif

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
    case Opcode::DIV: op_sdiv(i); break;
    case Opcode::DIVU: op_udiv(i); break;
    // Comparison
    case Opcode::SLT: set_rd(i, (s32)rs(i) < (s32)rt(i) ? 1 : 0); break;
    case Opcode::SLTU: set_rd(i, rs(i) < rt(i) ? 1 : 0); break;
    case Opcode::SLTI: set_rt(i, (s32)rs(i) < (s32)i.imm16_se() ? 1 : 0); break;
    case Opcode::SLTIU: set_rt(i, rs(i) < (u32)i.imm16_se() ? 1 : 0); break;
    // Shifting
    case Opcode::LUI: set_rt(i, i.imm16() << 16); break;
    case Opcode::SLLV: set_rd(i, rt(i) << (rs(i) & 0x1F)); break;
    case Opcode::SRLV: set_rd(i, rt(i) >> (rs(i) & 0x1F)); break;
    case Opcode::SRAV: set_rd(i, (s32)rt(i) >> (rs(i) & 0x1F)); break;
    case Opcode::SLL: set_rd(i, rt(i) << i.imm5()); break;
    case Opcode::SRL: set_rd(i, rt(i) >> i.imm5()); break;
    case Opcode::SRA: set_rd(i, (s32)rt(i) >> i.imm5()); break;
    // Logical
    case Opcode::ANDI: set_rt(i, rs(i) & i.imm16()); break;
    case Opcode::ORI: set_rt(i, rs(i) | i.imm16()); break;
    case Opcode::XORI: set_rt(i, rs(i) ^ i.imm16()); break;
    case Opcode::AND: set_rd(i, rs(i) & rt(i)); break;
    case Opcode::OR: set_rd(i, rs(i) | rt(i)); break;
    case Opcode::XOR: set_rd(i, rs(i) ^ rt(i)); break;
    case Opcode::NOR: set_rd(i, 0xFFFFFFFF ^ (rs(i) | rt(i))); break;
    // Memory operations (loads)
    case Opcode::LBU: issue_pending_load(i.rt(), read8(i.imm16_se() + rs(i))); break;
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
    case Opcode::JALR:
      set_rd(i, pc());
      pc() = rs(i);
      break;
    case Opcode::BEQ:
      if (rs(i) == rt(i))
        op_branch(i);
      break;
    case Opcode::BNE:
      if (rs(i) != rt(i))
        op_branch(i);
      break;
    case Opcode::BGTZ:
      if ((s32)rs(i) > 0)
        op_branch(i);
      break;
    case Opcode::BLEZ:
      if ((s32)rs(i) <= 0)
        op_branch(i);
      break;
    case Opcode::BCONDZ: {  // TODO: make separate opcodes?
      const u8 opcode_2 = i.rt();
      const bool is_bgez = opcode_2 & 0b1;
      const bool is_link = opcode_2 & 0b10000;

      // test if <= 0. invert result (>0) if is_bgez is true
      const auto test = (u32)((s32)rs(i) < 0) ^ is_bgez;
      if (test) {
        if (is_link) {
          r(31) = pc();
        }
        op_branch(i);
      }
      break;
    }

    // Co-processor 0
    case Opcode::MTC0: {
      const auto cop_dst_reg = static_cast<Cop0Register>(i.rd());
      switch (cop_dst_reg) {
        case Cop0Register::COP0_SR: m_sr = rt(i); break;
        default: LOG_WARN("Unhandled Cop1 register {} write", static_cast<u32>(cop_dst_reg));
      }
      break;
    }
    case Opcode::MFC0: {
      const auto cop_dst_reg = static_cast<Cop0Register>(i.rd());
      switch (cop_dst_reg) {
        case Cop0Register::COP0_SR: issue_pending_load(i.rt(), m_sr); break;
        default: LOG_WARN("Unhandled Cop1 register {} read", static_cast<u32>(cop_dst_reg));
      }
      break;
    }
    case Opcode::MFLO: set_rd(i, m_lo); break;
    case Opcode::MFHI: set_rd(i, m_hi); break;

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

void Cpu::op_udiv(const Instruction& i) {
  const u32 numerator = rs(i);
  const u32 denominator = rt(i);

  if (denominator == 0) {
    m_lo = 0xFFFFFFFF;
    m_hi = numerator;
  } else {
    m_lo = numerator / denominator;
    m_hi = numerator % denominator;
  }
}

void Cpu::op_sdiv(const Instruction& i) {
  const s32 numerator = rs(i);
  const s32 denominator = rt(i);

  if (denominator == 0) {
    if (numerator >= 0)
      m_lo = 0xFFFFFFFF;
    else
      m_lo = 1;
    m_hi = numerator;
  } else if (denominator == 0xFFFFFFFF && numerator == 0x80000000) {
    m_lo = 0x80000000;
    m_hi = 0;
  } else {
    m_lo = numerator / denominator;
    m_hi = numerator % denominator;
  }
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
