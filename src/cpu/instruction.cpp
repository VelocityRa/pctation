#include <cpu/cpu.hpp>
#include <cpu/instruction.hpp>

#include <spdlog/fmt/fmt.h>

namespace cpu {

Instruction::Instruction(u32 word) : m_word(word), m_opcode(decode()) {}

Opcode Instruction::decode() {
  const u8 primary_opcode = op_prim();

  if (primary_opcode != 0) {
    // non-SPECIAL opcode
    switch (primary_opcode) {
#define OPCODE_PRIM(mnemonic, opcode, operand1, operand2, operand3) \
  case opcode:                                                      \
    m_operands = { operand1, operand2, operand3 };                  \
    return Opcode::mnemonic;
#include <cpu/opcodes.def>
#undef OPCODE_PRIM
      default: return Opcode::INVALID;
    }
  } else {
    // SPECIAL opcode
    const u8 seconday_opcode = op_sec();
    switch (seconday_opcode) {
#define OPCODE_SEC(mnemonic, opcode, operand1, operand2, operand3) \
  case opcode:                                                     \
    m_operands = { operand1, operand2, operand3 };                 \
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

  for (const auto operand : m_operands) {
    switch (operand) {
      case OPERAND_NONE: break;
      case OPERAND_RS:
        disasm_text += fmt::format("{}, ", register_to_str(rs()));
        break;
      case OPERAND_RT:
        disasm_text += fmt::format("{}, ", register_to_str(rt()));
        break;
      case OPERAND_RD:
        disasm_text += fmt::format("{}, ", register_to_str(rd()));
        break;
      case OPERAND_IMM5: disasm_text += fmt::format("0x{:X}, ", imm5()); break;
      case OPERAND_IMM16:
        disasm_text += fmt::format("0x{:X}, ", imm16());
        break;
      case OPERAND_IMM20:
        disasm_text += fmt::format("0x{:X}, ", imm20());
        break;
      case OPERAND_IMM26:
        disasm_text += fmt::format("0x{:X}, ", imm26());
        break;
      default: disasm_text += "<invalid_operand>";
    }
  }

  // Erase trailing ", "
  if (disasm_text[disasm_text.size() - 2] == ',')
    disasm_text.erase(disasm_text.size() - 2, 2);

  return disasm_text;
}

}  // namespace cpu
