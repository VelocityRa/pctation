#pragma once

#include <cpu/opcode.hpp>
#include <util/types.hpp>

#define OPERAND_RS 1      // Register source
#define OPERAND_RT 2      // Register target
#define OPERAND_RD 4      // Register destination
#define OPERAND_IMM5 8    // Immediate 5 bits
#define OPERAND_IMM16 16  // Immediate 16 bits

namespace cpu {

class Instruction {
 public:
  explicit Instruction(u32 word);

  // Primary opcode field [31:26]
  constexpr u8 op_prim() const {
    return (m_word & 0b11111100'00000000'00000000'00000000) >> 26;
  }
  // Secondary opcode field [5:0]
  constexpr u8 op_sec() const {
    return (m_word & 0b00000000'00000000'00000000'00111111) >> 0;
  }
  // Register source field [25:21]
  constexpr u8 rs() const {
    return (m_word & 0b00000011'11100000'00000000'00000000) >> 21;
  }
  // Register target field [20:16]
  constexpr u8 rt() const {
    return (m_word & 0b00000000'00011111'00000000'00000000) >> 16;
  }
  // Register destination field [15:11]
  constexpr u8 rd() const {
    return (m_word & 0b00000000'00000000'11111000'00000000) >> 11;
  }
  // Immediate 5 field [10:6]
  constexpr u8 imm5() const {
    return (m_word & 0b00000000'00000000'00000111'11000000) >> 6;
  }
  // Immediate 16 field [16:0]
  constexpr u16 imm16() const {
    return (m_word & 0b00000000'00000000'11111111'11111111) >> 0;
  }

  constexpr Opcode opcode() const { return m_opcode; }

  std::string disassemble() const;

 private:
  Opcode decode();

  static const char* register_to_str(u8 reg_idx) {
    switch (reg_idx) {
      case 0: return "zero";
      case 1: return "at";
      case 2: return "v0";
      case 3: return "v1";
      case 4: return "a0";
      case 5: return "a1";
      case 6: return "a2";
      case 7: return "a3";
      case 8: return "t0";
      case 9: return "t1";
      case 10: return "t2";
      case 11: return "t3";
      case 12: return "t4";
      case 13: return "t5";
      case 14: return "t6";
      case 15: return "t7";
      case 16: return "s0";
      case 17: return "s1";
      case 18: return "s2";
      case 19: return "s3";
      case 20: return "s4";
      case 21: return "s5";
      case 22: return "s6";
      case 23: return "s7";
      case 24: return "t8";
      case 25: return "t9";
      case 26: return "k0";
      case 27: return "k1";
      case 28: return "gp";
      case 29: return "sp";
      case 30: return "fp";
      case 31: return "ra";
      default: return "<invalid>";
    }
  }

 private:
  const u32 m_word;
  u16 m_operands;  //  bitfield of enum Operand
  const Opcode m_opcode;
};

}  // namespace cpu
