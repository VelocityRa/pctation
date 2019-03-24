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

namespace cpu {
constexpr auto PC_RESET_ADDR = 0xBFC00000u;


class Cpu {
 public:
  explicit Cpu(bus::Bus& bus);
  bool step(u32& cycles_passed);
  void execute_instruction(const Instruction& i);

  u32 read32(u32 addr) const;
  void write32(u32 addr, u32 val);

 private:
  // Register getters
  const Register& r(RegisterIndex index) const {
    Ensures(index >= 0);
    Ensures(index <= 32);
    return m_gpr[index];
  }
  const Register& pc() const { return m_pc; }
  Register& pc() { return m_pc; }

  // Instruction operand register getters
  const Register& rs(const Instruction& i) const { return r(i.rs()); }
  const Register& rt(const Instruction& i) const { return r(i.rt()); }
  const Register& rd(const Instruction& i) const { return r(i.rd()); }

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

 private:
  bus::Bus& m_bus;

  // General purpose registers
  std::array<Register, 32> m_gpr{};

  // Special purpose registers
  Register m_pc{ PC_RESET_ADDR };

  // Next instruction to execute
  // Used to emulate branch delay slot
  u32 m_next_instr{ 0 };
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
