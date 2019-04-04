#include <bus/bus.hpp>
#include <cpu/cpu.hpp>
#include <cpu/instruction.hpp>
#include <cpu/opcode.hpp>
#include <util/log.hpp>

#include <climits>
#include <iostream>
#include <string>

#define TRACE_NONE 0
#define TRACE_INST 1
#define TRACE_REGS 2
#define TRACE_PC_ONLY 3

#define TRACE_MODE TRACE_NONE

#define TTY_OUTPUT 1

namespace cpu {

Cpu::Cpu(bus::Bus& bus) : m_bus(bus) {}

bool Cpu::step(u32& cycles_passed) {
  // Fetch current instruction
  const u32 cur_instr = m_bus.read32(m_pc);

  // Decode current instruction
  const Instruction instr(cur_instr);

  // Log instruction disassembly
#if TRACE_MODE == TRACE_REGS
  std::string debug_str;
  for (auto i = 1; i < 32; ++i)
    debug_str +=
        fmt::format("{}:{:X} ", reinterpret_cast<const char*>(register_to_str(i)) + 1, m_gpr[i]);
  debug_str += fmt::format("hi:{:X} lo:{:X}", m_hi, m_lo);
  LOG_TRACE("[{:08X}]: {:08X} {}\n  {}", m_previous_pc, cur_instr, instr.disassemble(), debug_str);
#elif TRACE_MODE == TRACE_INST
  LOG_TRACE("[{:08X}]: {:08X} {}", m_pc, cur_instr, instr.disassemble());
#endif

  // Check PC alignment
  if (m_pc % 4 != 0) {
    trigger_exception(ExceptionCause::EC_LOAD_ADDR_ERR);
    return false;
  }

  // Save current PC
  m_pc_current = m_pc;
  // Advance PC
  m_pc = m_pc_next;
  m_pc_next += 4;

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
    case Opcode::MULT: op_mult(i); break;
    case Opcode::MULTU: op_multu(i); break;
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
    case Opcode::NOR: set_rd(i, !(rs(i) | rt(i))); break;
    // Memory operations (loads)
    case Opcode::LBU: op_lbu(i); break;
    case Opcode::LB: op_lb(i); break;
    case Opcode::LHU: op_lhu(i); break;
    case Opcode::LH: op_lh(i); break;
    case Opcode::LW:
      op_lw(i);
      break;
      // Memory operations (stores)
    case Opcode::SB: op_sb(i); break;
    case Opcode::SH: op_sh(i); break;
    case Opcode::SW: op_sw(i); break;
    // Jumps/Branches
    case Opcode::J: op_jump(i); break;
    case Opcode::JR: m_pc_next = rs(i);
#if TTY_OUTPUT
      // Hook std_out_putchar function in the B0 kernel table. Assumes there's an ADDIU/"LI" instruction
      // right after the JR, containing the kernel procedure vector.
      if (m_pc_next == 0xB0) {
        const Instruction next_instr(load32(m_pc));
        Ensures(next_instr.opcode() == Opcode::ADDIU);

        if (next_instr.imm16_se() == 0x3D) {
          char tty_out_char = r(4);
          std::cout << tty_out_char;
        }
      }
#endif
      break;
    case Opcode::JAL:
      r(31) = m_pc_next;
      op_jump(i);
      break;
    case Opcode::JALR:
      set_rd(i, m_pc_next);
      m_pc_next = rs(i);
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
          r(31) = m_pc_next;
        }
        op_branch(i);
      }
      break;
    }
    // Syscall
    case Opcode::SYSCALL: trigger_exception(ExceptionCause::EC_SYSCALL); break;
    // Co-processor 0
    case Opcode::MTC0: {
      const auto cop_dst_reg = static_cast<Cop0Register>(i.rd());
      switch (cop_dst_reg) {
        case Cop0Register::COP0_SR: m_cop0_sr = rt(i); break;
        case Cop0Register::COP0_CAUSE: m_cop0_cause = rt(i); break;
        case Cop0Register::COP0_EPC: m_cop0_epc = rt(i); break;
        default: LOG_WARN("Unhandled Cop1 register {} write", static_cast<u32>(cop_dst_reg));
      }
      break;
    }
    case Opcode::MFC0: {
      const auto cop_dst_reg = static_cast<Cop0Register>(i.rd());
      switch (cop_dst_reg) {
        case Cop0Register::COP0_SR: issue_pending_load(i.rt(), m_cop0_sr); break;
        case Cop0Register::COP0_CAUSE: issue_pending_load(i.rt(), m_cop0_cause); break;
        case Cop0Register::COP0_EPC: issue_pending_load(i.rt(), m_cop0_epc); break;
        default: LOG_WARN("Unhandled Cop1 register {} read", static_cast<u32>(cop_dst_reg));
      }
      break;
    }
    case Opcode::MFLO: set_rd(i, m_lo); break;
    case Opcode::MFHI: set_rd(i, m_hi); break;
    case Opcode::MTLO: m_lo = rs(i); break;
    case Opcode::MTHI: m_hi = rs(i); break;
    case Opcode::RFE: op_rfe(i); break;

    default: LOG_ERROR("Unimplemented instruction executed"); assert(0);
  }
  check_pending_load();
}

void Cpu::trigger_exception(ExceptionCause cause) {
  const u32 handler_addr =
      m_cop0_sr & COP0_SR_BEV ? EXCEPTION_VECTOR_GENERAL_ROM : EXCEPTION_VECTOR_GENERAL_RAM;

  // Shift bits [5:0] of the Status Register two bits to the left.
  // This has the effect of disabling interrupts and enabling kernel mode.
  // The last two bits in [5:4] are discarded, it's up to the kernel to handle more exception recursion
  // levels.
  const u8 mode = m_cop0_sr & 0b111111;
  m_cop0_sr &= ~0b111111u;
  m_cop0_sr |= (mode << 2) & 0b111111;

  m_cop0_cause = (static_cast<u32>(cause) << 2);

  // Update EPC with the return address (PC) from the exception
  m_cop0_epc = m_pc_current;
  if (is_in_branch_delay_slot()) {
    m_cop0_epc = m_pc_next;   // need to set this to the branch target if we're in a branch delay slot
    m_cop0_sr |= COP0_SR_BD;  // also set this SR bit which indicates this edge case
  }

  // Exceptions don't have a branch delay, jump directly to the handler
  m_pc = handler_addr;
  m_pc_next = m_pc + 4;
}

void Cpu::op_sb(const Instruction& i) {
  const address addr = i.imm16_se() + rs(i);
  return store8(addr, (u8)rt(i));
}

void Cpu::op_sh(const Instruction& i) {
  const address addr = i.imm16_se() + rs(i);
  if (addr % 2 != 0) {
    trigger_exception(ExceptionCause::EC_STORE_ADDR_ERR);
    return;
  }
  store16(addr, (u16)rt(i));
}

void Cpu::op_sw(const Instruction& i) {
  const address addr = i.imm16_se() + rs(i);
  if (addr % 4 != 0) {
    trigger_exception(ExceptionCause::EC_STORE_ADDR_ERR);
    return;
  }
  store32(addr, rt(i));
}

void Cpu::op_lbu(const Instruction& i) {
  const address addr = i.imm16_se() + rs(i);
  issue_pending_load(i.rt(), load8(addr));
}

void Cpu::op_lb(const Instruction& i) {
  const address addr = i.imm16_se() + rs(i);
  issue_pending_load(i.rt(), (s8)load8(addr));
}

void Cpu::op_lhu(const Instruction& i) {
  const address addr = i.imm16_se() + rs(i);
  if (addr % 2 != 0) {
    trigger_exception(ExceptionCause::EC_LOAD_ADDR_ERR);
    return;
  }
  issue_pending_load(i.rt(), load16(addr));
}

void Cpu::op_lh(const Instruction& i) {
  const address addr = i.imm16_se() + rs(i);
  if (addr % 2 != 0) {
    trigger_exception(ExceptionCause::EC_LOAD_ADDR_ERR);
    return;
  }
  issue_pending_load(i.rt(), (s16)load16(addr));
}

void Cpu::op_lw(const Instruction& i) {
  const address addr = i.imm16_se() + rs(i);
  if (addr % 4 != 0) {
    trigger_exception(ExceptionCause::EC_LOAD_ADDR_ERR);
    return;
  }
  issue_pending_load(i.rt(), load32(addr));
}

void Cpu::op_jump(const Instruction& i) {
  m_pc_next = m_pc_next & 0xF0000000 | (i.imm26() << 2);
}

void Cpu::op_mult(const Instruction& i) {
  const auto a = (s64)(s32)rs(i);
  const auto b = (s64)(s32)rt(i);
  const auto result = (u64)(a * b);

  m_hi = result >> 32;
  m_lo = (u32)result;
}

void Cpu::op_multu(const Instruction& i) {
  const auto result = (u64)rs(i) * (u64)rt(i);
  m_hi = result >> 32;
  m_lo = (u32)result;
}

void Cpu::op_branch(const Instruction& i) {
  m_pc_next += (i.imm16_se() << 2) - 4;
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

void Cpu::op_rfe(const Instruction& i) {
  // Restore the mode before the exception by shifting the Interrupt Enable / User Mode stack back to its
  // original position.
  const auto mode = m_cop0_sr & 0b111111;

  m_cop0_sr &= ~0b1111u;
  m_cop0_sr |= mode >> 2;
}

u32 Cpu::checked_add(s32 op1, s32 op2) {
  if (op1 > 0 && op2 > 0 && op1 > INT_MAX - op2 || op1 < 0 && op2 < 0 && op1 < INT_MIN - op2)
    trigger_exception(ExceptionCause::EC_OVERFLOW);
  return op1 + op2;
}

u32 Cpu::checked_sub(s32 op1, s32 op2) {
  if (op1 < 0 && op2 > INT_MAX + op1 || op1 > 0 && op2 < INT_MIN + op1)
    trigger_exception(ExceptionCause::EC_OVERFLOW);
  return op1 - op2;
}

// u32 Cpu::checked_mul(u32 op1, u32 op2) {
//  if (static_cast<u64>(op1) * static_cast<u64>(op2) > 0x100000000LL)
//    trigger_exception(ExceptionCause::EC_OVERFLOW);
//  return op1 - op2;
//}

u32 Cpu::load32(u32 addr) {
  if (addr % 4 != 0) {
    trigger_exception(ExceptionCause::EC_LOAD_ADDR_ERR);
    return 0;
  }
  return m_bus.read32(addr);
}

u16 Cpu::load16(u32 addr) {
  if (addr % 2 != 0) {
    trigger_exception(ExceptionCause::EC_LOAD_ADDR_ERR);
    return 0;
  }
  return m_bus.read16(addr);
}

u8 Cpu::load8(u32 addr) {
  return m_bus.read8(addr);
}

void Cpu::store32(u32 addr, u32 val) {
  if (addr % 4 != 0) {
    trigger_exception(ExceptionCause::EC_STORE_ADDR_ERR);
    return;
  }
  if (m_cop0_sr & COP0_SR_ISOLATE_CACHE) {
    LOG_TRACE("Ignoring write 0x{:08X} to 0x{:08X} due to cache isolation", val, addr);
    return;
  }
  m_bus.write32(addr, val);
}
void Cpu::store16(u32 addr, u16 val) {
  if (addr % 2 != 0) {
    trigger_exception(ExceptionCause::EC_STORE_ADDR_ERR);
    return;
  }
  if (m_cop0_sr & COP0_SR_ISOLATE_CACHE) {
    LOG_TRACE("Ignoring write 0x{:04X} to 0x{:08X} due to cache isolation", val, addr);
    return;
  }
  m_bus.write16(addr, val);
}

void Cpu::store8(u32 addr, u8 val) {
  if (m_cop0_sr & COP0_SR_ISOLATE_CACHE) {
    LOG_TRACE("Ignoring write 0x{:02X} to 0x{:08X} due to cache isolation", val, addr);
    return;
  }
  m_bus.write8(addr, val);
}

}  // namespace cpu
