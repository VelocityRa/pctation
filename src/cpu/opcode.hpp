#pragma once

namespace cpu {

enum class Opcode {
#define OPCODE(mnemonic, value) mnemonic,
#include <cpu/opcodes.def>
#undef OPCODE
  INVALID
};

inline const char* opcode_to_str(Opcode opcode) {
  switch (opcode) {
#define OPCODE(mnemonic, value) \
  case Opcode::mnemonic:        \
    return #mnemonic;
#include <cpu/opcodes.def>
#undef OPCODE
    default: return "<INVALID>";
  }
}

}  // namespace cpu
