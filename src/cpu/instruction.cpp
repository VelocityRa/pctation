#include <cpu/instruction.hpp>

#include <spdlog/fmt/fmt.h>

namespace cpu {

Instruction::Instruction(u32 word)
    : m_word(word), m_operands(0), m_opcode(decode()) {}

Opcode Instruction::decode() {
  const u8 primary_opcode = op_prim();

  if (primary_opcode != 0) {
    // non-SPECIAL opcode
    switch (primary_opcode) {
#define OPCODE_PRIM(mnemonic, value, operands) \
  case value:                                  \
    m_operands = operands;                     \
    return Opcode::mnemonic;
#include <cpu/opcodes.def>
#undef OPCODE_PRIM
      default: return Opcode::INVALID;
    }
  } else {
    // SPECIAL opcode
    const u8 seconday_opcode = op_sec();
    switch (seconday_opcode) {
#define OPCODE_SEC(mnemonic, value, operands) \
  case value:                                 \
    m_operands = operands;                    \
    return Opcode::mnemonic;
#include <cpu/opcodes.def>
#undef OPCODE_SEC
      default: return Opcode::INVALID;
    }
  }
}

std::string Instruction::disassemble() const {
  std::string disasm_text;
  disasm_text.reserve(32);

  // Opcode
  disasm_text = opcode_to_str(m_opcode);
  disasm_text += "\t";

  // TODO: some instructions' operands should be printed in a different order
  // TODO: add 3 operand fields to opcodes.def? or maybe add 1 more (an enum) to
  // choose between hardcoded orderings?
  if (m_operands & OPERAND_RD)
    disasm_text += fmt::format("{}, ", register_to_str(rd()));
  if (m_operands & OPERAND_RS)
    disasm_text += fmt::format("{}, ", register_to_str(rs()));
  if (m_operands & OPERAND_RT)
    disasm_text += fmt::format("{}, ", register_to_str(rt()));
  if (m_operands & OPERAND_IMM16)
    disasm_text += fmt::format("0x{:X}, ", imm16());

  disasm_text.erase(disasm_text.size() - 2, 2);

  return disasm_text;
}

}  // namespace cpu
