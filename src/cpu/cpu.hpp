#pragma once

#include <cpu/instruction.hpp>
#include <util/types.hpp>

#include <gsl-lite.hpp>

#include <array>

namespace bus {
class Bus;
}

namespace cpu {
class Instruction;
}

#define TTY_OUTPUT true

namespace cpu {

constexpr auto PC_RESET_ADDR = 0xBFC00000u;

// Interrupt vectors for general interrupts and exceptions
constexpr auto EXCEPTION_VECTOR_GENERAL_RAM = 0x80000080u;
constexpr auto EXCEPTION_VECTOR_GENERAL_ROM = 0xBFC00180u;

// Co-processor 0 registers
enum class Cop0Register : u32 {
  COP0_BPC = 3,        // BPC - Breakpoint on execute (R/W)
  COP0_BDA = 5,        // BDA - Breakpoint on data access (R/W)
  COP0_JUMPDEST = 6,   // JUMPDEST - Randomly memorized jump address (R)
  COP0_DCIC = 7,       // DCIC - Breakpoint control (R/W)
  COP0_BAD_VADDR = 8,  // BadVaddr - Bad Virtual Address (R)
  COP0_BDAM = 9,       // BDAM - Data Access breakpoint mask (R/W)
  COP0_BPCM = 11,      // BPCM - Execute breakpoint mask (R/W)
  COP0_SR = 12,        // SR - System status register (R/W)
  COP0_CAUSE = 13,     // CAUSE - (R)  Describes the most recently recognised exception
  COP0_EPC = 14,       // EPC - Return Address from Trap (R)
  COP0_PRID = 15,      // PRID - Processor ID (R)
};

// Co-processor 0 Status Register fields
// See usages for details
enum Cop0StatusRegister {
  // First 6 bits are used directly in Cpu::trigger_excption
  COP0_SR_ISOLATE_CACHE = 1 << 16,
  COP0_SR_BEV = 1 << 22,
  COP0_SR_BD = 1 << 31,
};

enum class ExceptionCause : u32 {
  LoadAddressError = 0x4,
  StoreAddressError = 0x5,
  Syscall = 0x8,
  Overflow = 0xC,
};

class Cpu {
 public:
  explicit Cpu(bus::Bus& bus);

  bool step(u32& cycles_passed);

  // Getters
  const std::string& tty_out() const { return m_tty_out; }

 private:
  void execute_instruction(const Instruction& i);
  void trigger_exception(ExceptionCause cause);

  // Interpreter helpers
  void op_lbu(const Instruction& i);
  void op_sb(const Instruction& i);
  void op_sh(const Instruction& i);
  void op_sw(const Instruction& i);
  void op_swl(const Instruction& i);
  void op_swr(const Instruction& i);
  void op_lb(const Instruction& i);
  void op_lhu(const Instruction& i);
  void op_lh(const Instruction& i);
  void op_lw(const Instruction& i);
  void op_lwl(const Instruction& i);
  void op_lwr(const Instruction& i);
  void op_branch(const Instruction& i);
  void op_jump(const Instruction& i);
  void op_mult(const Instruction& i);
  void op_multu(const Instruction& i);
  void op_udiv(const Instruction& i);
  void op_sdiv(const Instruction& i);
  void op_rfe(const Instruction& i);

  u32 load32(u32 addr);
  u16 load16(u32 addr);
  u8 load8(u32 addr);
  void store32(u32 addr, u32 val);
  void store16(u32 addr, u16 val);
  void store8(u32 addr, u8 val);
  u32 checked_add(s32 op1, s32 op2);
  u32 checked_sub(s32 op1, s32 op2);
  // u32 checked_mul(u32 op1, u32 op2);

 private:
  // Register getters/setters
  const Register& r(RegisterIndex index) const {
    //    Ensures(index >= 0);
    //    Ensures(index <= 32);
    return m_gpr[index];
  }
  Register& r(RegisterIndex index) {
    //    Ensures(index >= 0);
    //    Ensures(index <= 32);
    return m_gpr[index];
  }

  // Instruction operand register getters
  const Register& rs(const Instruction& i) const { return r(i.rs()); }
  const Register& rt(const Instruction& i) const { return r(i.rt()); }
  const Register& rd(const Instruction& i) const { return r(i.rd()); }

  // Load delay emulation

  struct DelayedLoadInfo {
    bool is_pending;
    u8 time_left;
    RegisterIndex reg;
    u32 val;
  };

  void issue_pending_load(RegisterIndex reg, u32 val) {
    // Commit the pending load if any
    check_pending_load();

    m_pending_load.reg = reg;
    m_pending_load.val = val;
    m_pending_load.time_left = 1;
    m_pending_load.is_pending = true;
  }

  void check_pending_load() {
    if (m_pending_load.is_pending) {
      if (m_pending_load.time_left == 0) {
        m_gpr[m_pending_load.reg] = m_pending_load.val;
        m_pending_load.is_pending = false;
      } else
        m_pending_load.time_left--;
    }
  }

  // Instruction operand register setters
  void set_rs(const Instruction& i, u32 v) {
    m_gpr[i.rs()] = v;
    m_gpr[0] = 0;
  }
  void set_rt(const Instruction& i, u32 v) {
    m_gpr[i.rt()] = v;
    m_gpr[0] = 0;
  }
  void set_rd(const Instruction& i, u32 v) {
    m_gpr[i.rd()] = v;
    m_gpr[0] = 0;
  }

  // Branch delay slot helpers
  bool is_in_branch_delay_slot() const {
    // TODO: Would miss an edge case where there's a jump for 4 bytes ahead
    return m_pc + 4 == m_pc_next;
  }

 private:
  // General purpose registers
  std::array<Register, 32> m_gpr{};

  // Special purpose registers
  Register
      m_pc_current{};  // Previous program counter. Used when an exception happens (saved to COP0_EPC).
  Register m_pc{ PC_RESET_ADDR };  // Current PC
  Register m_pc_next{ m_pc + 4 };  // Next PC we'll execute (used for branch delay emulation)
  Register m_hi{};                 // Division remainder and multiplication high result
  Register m_lo{};                 // Division quotient and multiplication low result

  // Cop0 registers
  Register m_cop0_sr{};     // Status Register (r12)
  Register m_cop0_cause{};  // Cause Register (r13)
  Register m_cop0_epc{};    // EPC (r14)

  // For emulating Load delay
  DelayedLoadInfo m_pending_load{};

  //  #ifndef NDEBUG
  //  // For debugging
  //  u64 instr_counter{};
  //  #endif

  bus::Bus& m_bus;

#ifdef TTY_OUTPUT
  std::string m_tty_out;
#endif
};

static const char* register_to_str(u8 reg_idx) {
  switch (reg_idx) {
    case 0: return "$zero";
    case 1: return "$at";
    case 2: return "$v0";
    case 3: return "$v1";
    case 4: return "$a0";
    case 5: return "$a1";
    case 6: return "$a2";
    case 7: return "$a3";
    case 8: return "$t0";
    case 9: return "$t1";
    case 10: return "$t2";
    case 11: return "$t3";
    case 12: return "$t4";
    case 13: return "$t5";
    case 14: return "$t6";
    case 15: return "$t7";
    case 16: return "$s0";
    case 17: return "$s1";
    case 18: return "$s2";
    case 19: return "$s3";
    case 20: return "$s4";
    case 21: return "$s5";
    case 22: return "$s6";
    case 23: return "$s7";
    case 24: return "$t8";
    case 25: return "$t9";
    case 26: return "$k0";
    case 27: return "$k1";
    case 28: return "$gp";
    case 29: return "$sp";
    case 30: return "$fp";
    case 31: return "$ra";
    default: return "$<invalid>";
  }
}

}  // namespace cpu
