#include <cpu/instruction.hpp>

namespace cpu {

Instruction::Instruction(u32 word) : m_word(word) {
  m_opcode = decode_opcode();
}

Opcode Instruction::decode_opcode() const {
  u8 primary_opcode = op_prim();

  if (primary_opcode != 0) {
    // non-SPECIAL opcode
    switch (primary_opcode) {
#define OPCODE_PRIM(mnemonic, value) \
  case value:                        \
    return Opcode::mnemonic;
#include <cpu/opcodes.def>
#undef OPCODE_PRIM
      default: return Opcode::INVALID;
    }
  } else {
    // SPECIAL opcode
    u8 seconday_opcode = op_sec();
    switch (seconday_opcode) {
#define OPCODE_SEC(mnemonic, value) \
  case value:                       \
    return Opcode::mnemonic;
#include <cpu/opcodes.def>
#undef OPCODE_SEC
      default: return Opcode::INVALID;
    }
  }
}

}  // namespace cpu
