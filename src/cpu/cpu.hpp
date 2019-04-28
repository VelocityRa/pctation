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

#define TTY_OUTPUT 1

namespace cpu {

constexpr u32 APPROX_CYCLES_PER_INSTRUCTION = 2;

constexpr auto PC_RESET_ADDR = 0xBFC00000u;

// Interrupt vectors for general interrupts and exceptions
constexpr auto EXCEPTION_VECTOR_GENERAL_RAM = 0x80000080u;
constexpr auto EXCEPTION_VECTOR_GENERAL_ROM = 0xBFC00180u;
// Interrupt vector for breakpoints
constexpr auto EXCEPTION_VECTOR_BREAKPOINT = 0x80000040u;

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

union Cop0CauseRegister {
  u32 word;
  struct {
    u32 _0_7 : 8;
    u32 interrupt_pending : 8;
    u32 _16_30 : 14;
    u32 branch_delay_taken : 1;
    u32 branch_delay : 1;
  };
};

union Cop0StatusRegister {
  u32 word;
  struct {
    u32 interrupt_enable : 1;
    u32 _1_7 : 7;
    u32 interrupt_mask : 8;
    u32 isolate_cache : 1;
    u32 _17_21 : 5;
    u32 boot_exception_vectors : 1;
    u32 _22_31 : 9;
  };
};

enum class ExceptionCause : u32 {
  Interrupt = 0x0,
  LoadAddressError = 0x4,
  StoreAddressError = 0x5,
  Syscall = 0x8,
  Breakpoint = 0x9,
  Overflow = 0xC,
};

class Cpu {
  friend class Interrupts;

 public:
  explicit Cpu(bus::Bus& bus);

  void step(u32 cycles_to_execute);

  // Getters
  const std::string& tty_out() const { return m_tty_out; }

 private:
  void execute_instruction(const Instruction& i);

  // Exceptions
  void store_exception_state();
  void trigger_exception(ExceptionCause cause);
  void trigger_load_exception(const address addr);
  void trigger_store_exception(const address addr);

  // Interpreter helpers
  void op_add(const Instruction& i);
  void op_sub(const Instruction& i);
  void op_addi(const Instruction& i);
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
  void op_j(const Instruction& i);
  void op_jr(const Instruction& i);
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
  bool checked_add(u32 op1, u32 op2, u32& out);  // returns false on overflow
  bool checked_sub(u32 op1, u32 op2, u32& out);  // returns false on overflow

 private:
  // Register getters/setters
  Register gpr(RegisterIndex index) const {
    //    Ensures(index >= 0);
    //    Ensures(index <= 31);
    return m_gpr[index];
  }
  Register& gpr(RegisterIndex index) {
    //    Ensures(index >= 0);
    //    Ensures(index <= 31);
    return m_gpr[index];
  }
  void set_gpr(RegisterIndex index, u32 v) {
    m_gpr[index] = v;
    m_gpr[0] = 0;
  }
  void set_pc(address addr) {
    m_pc = addr;
    m_pc_next = m_pc + 4;
  }
  void set_pc_next(address addr) {
    m_pc_next = addr;
    m_branch_taken = true;
  }

  // Instruction operand register getters
  Register rs(const Instruction& i) const { return gpr(i.rs()); }
  Register rt(const Instruction& i) const { return gpr(i.rt()); }
  Register rd(const Instruction& i) const { return gpr(i.rd()); }

  // Instruction operand register setters
  void set_rs(const Instruction& i, u32 v) {
    set_gpr(i.rs(), v);
    invalidate_reg(i.rs());
  }
  void set_rt(const Instruction& i, u32 v) {
    set_gpr(i.rt(), v);
    invalidate_reg(i.rt());
  }
  void set_rd(const Instruction& i, u32 v) {
    set_gpr(i.rd(), v);
    invalidate_reg(i.rd());
  }

 private:
  // General purpose registers
  std::array<Register, 32> m_gpr{};

  // Special purpose registers
  Register m_pc_current{};         // Previous program counter. Used when an exception happens
                                   // (saved to COP0_EPC).
  Register m_pc{ PC_RESET_ADDR };  // Current PC
  Register m_pc_next{ m_pc + 4 };  // Next PC we'll execute (used for branch delay emulation)
  Register m_hi{};                 // Division remainder and multiplication high result
  Register m_lo{};                 // Division quotient and multiplication low result

  // Cop0 registers (see Cop0Register for descriptions)
  Register m_cop0_bpc{};
  Register m_cop0_bda{};
  Register m_cop0_jumpdest{};
  Register m_cop0_dcic{};
  Register m_cop0_bad_vaddr{};
  Register m_cop0_bdam{};
  Register m_cop0_bpcm{};
  Cop0StatusRegister m_cop0_status{};
  Cop0CauseRegister m_cop0_cause{};
  Register m_cop0_epc{};
  Register m_cop0_prid{};

  // Load delay emulation
  void issue_delayed_load(RegisterIndex reg, u32 val);
  void do_pending_load();
  void invalidate_reg(RegisterIndex r);

  struct DelayedLoad {
    RegisterIndex reg{};
    u32 val{};
    u32 val_prev{};

    void invalidate() { reg = 0; }
    bool is_valid() const { return reg != 0; }
  };
  DelayedLoad m_slot_current{};
  DelayedLoad m_slot_next{};

  // Exceptions
  bool m_branch_taken{};
  bool m_branch_taken_saved{};
  bool m_in_branch_delay_slot{};
  bool m_in_branch_delay_slot_saved{};

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
