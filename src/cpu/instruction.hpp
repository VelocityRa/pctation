#pragma once

#include <cpu/opcode.hpp>
#include <util/types.hpp>

namespace cpu {

class Instruction {
 public:
  explicit Instruction(u32 word);

  // Primary opcode field [31:26]
  u8 op_prim() const {
    return (m_word & 0b11111100'00000000'00000000'00000000) >> 26;
  }
  // Secondary opcode field [5:0]
  u8 op_sec() const {
    return (m_word & 0b00000000'00000000'00000000'00111111) >> 0;
  }
  // Register source field [25:21]
  u8 rs() const {
    return (m_word & 0b00000011'11100000'00000000'00000000) >> 21;
  }
  // Register target field [20:16]
  u8 rt() const {
    return (m_word & 0b00000000'00011111'00000000'00000000) >> 16;
  }
  // Register destination field [15:11]
  u8 rd() const {
    return (m_word & 0b00000000'00000000'11111000'00000000) >> 11;
  }
  // Immediate 5 field [10:6]
  u8 imm5() const {
    return (m_word & 0b00000000'00000000'00000111'11000000) >> 6;
  }
  // Immediate 16 field [16:0]
  u8 imm16() const {
    return (m_word & 0b00000000'00000000'11111111'11111111) >> 0;
  }

  Opcode opcode() const { return m_opcode; }

 private:
  Opcode decode_opcode() const;

 private:
  u32 m_word;
  Opcode m_opcode;
};

}  // namespace cpu
