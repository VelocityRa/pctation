#include <cpu/cpu.hpp>
#include <cpu/instruction.hpp>

#include <spdlog/fmt/fmt.h>

namespace cpu {

Instruction::Instruction(u32 word) : m_word(word), m_opcode(decode()) {}

// TODO: Using X-Macros for instruction decoding & disassembly sounded like a good idea initially, but it
// got a bit out of hand. This should be refactored, if not to completely remove X-Macros use, to remove
// the disassembly part of it for some simplicty.

Opcode Instruction::decode() {
  const auto primary_opcode = op_prim();

  const bool is_cop_instr = m_word >> 30 & 1;

  // Disable formattinig to keep macro defs in a single line for brevity
  // clang-format off

  if (is_cop_instr) {  // Co-processor insruction
    const auto cop_opcode = op_cop();

    if (is_cop2_cmd()) {
      m_operands = { OPERAND_IMM25, OPERAND_NONE, OPERAND_NONE };
      return Opcode::COP2;
    }
    else if (cop_opcode == 0b010000'1'0000) { // COP0 instruction
      const u16 cop0_opcode_sec = op_cop0_sec();

      // RFE is the only COP0 instruction implemented on PlayStation hardware with this encoding
      Ensures(cop0_opcode_sec == 0b010000);

      return Opcode::RFE;
    } else {
      switch (cop_opcode) {  // Other COPn instruction
#define OPCODE_COP(mnemonic, opcode, operand1, operand2, operand3) \
  case opcode: m_operands = { operand1, operand2, operand3 }; return Opcode::mnemonic;
#include <cpu/opcodes.def>
#undef OPCODE_COP
        default: goto prim_encoding; // LWC/SWC instructions use this encoding (op is first 6 bits)
      }
    }
  } else if (primary_opcode != 0) {  // non-SPECIAL insruction
  prim_encoding:
    switch (primary_opcode) {
#define OPCODE_PRIM(mnemonic, opcode, operand1, operand2, operand3) \
  case opcode: m_operands = { operand1, operand2, operand3 }; return Opcode::mnemonic;
#include <cpu/opcodes.def>
#undef OPCODE_PRIM
      default: return Opcode::INVALID;
    }
  } else {  // SPECIAL insruction
    const u8 secondary_opcode = op_sec();

    switch (secondary_opcode) {
#define OPCODE_SEC(mnemonic, opcode, operand1, operand2, operand3) \
  case opcode: m_operands = { operand1, operand2, operand3 }; return Opcode::mnemonic;
#include <cpu/opcodes.def>
#undef OPCODE_SEC
      default: return Opcode::INVALID;
    }
  }
  // clang-format on
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
      case OPERAND_RS: disasm_text += fmt::format("{}, ", register_to_str(rs())); break;
      case OPERAND_RT: disasm_text += fmt::format("{}, ", register_to_str(rt())); break;
      case OPERAND_RD: disasm_text += fmt::format("{}, ", register_to_str(rd())); break;
      case OPERAND_IMM5: disasm_text += fmt::format("0x{:X}, ", imm5()); break;
      case OPERAND_IMM16: disasm_text += fmt::format("0x{:X}, ", imm16()); break;
      case OPERAND_IMM20: disasm_text += fmt::format("0x{:X}, ", imm20()); break;
      case OPERAND_IMM25: disasm_text += fmt::format("0x{:X}, ", imm25()); break;
      case OPERAND_IMM26: disasm_text += fmt::format("0x{:X}, ", imm26()); break;
      case OPERAND_GTE_REG:  // fall-through
      case OPERAND_GTE_GD: disasm_text += fmt::format("{}, ", gte::reg_to_str(rd())); break;
      case OPERAND_GTE_GC: disasm_text += fmt::format("{}, ", gte::reg_to_str(rd() + 32)); break;
      default: disasm_text += "<invalid_operand>";
    }
  }

  // Erase trailing ", "
  if (disasm_text[disasm_text.size() - 2] == ',')
    disasm_text.erase(disasm_text.size() - 2, 2);

  return disasm_text;
}

}  // namespace cpu
