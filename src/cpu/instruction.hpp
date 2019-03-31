#pragma once

#include <cpu/opcode.hpp>
#include <util/types.hpp>

#include <array>

#define OPERAND_NONE 0    // No operand
#define OPERAND_RS 1      // Register source
#define OPERAND_RT 2      // Register target
#define OPERAND_RD 4      // Register destination
#define OPERAND_IMM5 8    // Immediate 5 bits
#define OPERAND_IMM16 16  // Immediate 16 bits
#define OPERAND_IMM20 20  // Immediate 20 bits
#define OPERAND_IMM26 26  // Immediate 26 bits

namespace cpu {

using Register = u32;
using RegisterIndex = u8;

class Instruction {
 public:
  explicit Instruction(u32 word);

  // Primary opcode field [31:26]
  constexpr u8 op_prim() const { return (m_word & 0b11111100'00000000'00000000'00000000) >> 26; }
  // Secondary opcode field [5:0]
  constexpr u8 op_sec() const { return (m_word & 0b00000000'00000000'00000000'00111111) >> 0; }
  // Co-processor opcode field [31:21]
  constexpr u16 op_cop() const { return (m_word & 0b11111111'11100000'00000000'00000000) >> 21; }
  // Co-processor 0 secondary opcode field [5:0]
  constexpr u8 op_cop0_sec() const { return (m_word & 0b00000000'00000000'00000000'00111111) >> 0; }
  // Register source field [25:21]
  constexpr RegisterIndex rs() const { return (m_word & 0b00000011'11100000'00000000'00000000) >> 21; }
  // Register target field [20:16]
  constexpr RegisterIndex rt() const { return (m_word & 0b00000000'00011111'00000000'00000000) >> 16; }
  // Register destination field [15:11]
  constexpr RegisterIndex rd() const { return (m_word & 0b00000000'00000000'11111000'00000000) >> 11; }
  // Immediate 5 field [10:6]
  constexpr u8 imm5() const { return (m_word & 0b00000000'00000000'00000111'11000000) >> 6; }
  // Immediate 16 field [16:0]
  constexpr u16 imm16() const { return (m_word & 0b00000000'00000000'11111111'11111111) >> 0; }
  // Immediate 16 field [16:0]
  constexpr s16 imm16_se() const { return (m_word & 0b00000000'00000000'11111111'11111111) >> 0; }
  // Immediate 20 field [20:0]
  constexpr u32 imm20() const { return (m_word & 0b00000000'00001111'11111111'11111111) >> 0; }
  // Immediate 26 field [26:0]
  constexpr u32 imm26() const { return (m_word & 0b00000011'11111111'11111111'11111111) >> 0; }

  constexpr Opcode opcode() const { return m_opcode; }

  std::string disassemble() const;

 private:
  Opcode decode();

 private:
  const u32 m_word;
  const Opcode m_opcode;
  std::array<u16, 3> m_operands;  //  bitfield of enum Operand
};

}  // namespace cpu
