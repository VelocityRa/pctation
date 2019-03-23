#pragma once

namespace cpu {

enum class Opcode {
#define OPCODE(mnemonic, value, operands) mnemonic,
#include <cpu/opcodes.def>
#undef OPCODE
  INVALID
};

inline const char* opcode_to_str(Opcode opcode) {
  switch (opcode) {
#define OPCODE(mnemonic, value, operands) \
  case Opcode::mnemonic:                  \
    return #mnemonic;
#include <cpu/opcodes.def>
#undef OPCODE
    default: return "<INVALID>";
  }
}

}  // namespace cpu
