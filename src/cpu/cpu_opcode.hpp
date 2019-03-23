#pragma once

namespace cpu {

enum class Opcode {
#define OPCODE(mnemonic, value) mnemonic,
#include <cpu_opcodes.def>
#undef OPCODE
  UNKNOWN
};

inline const char* opcode_to_str(Opcode opcode) {
  switch (opcode) {
#define OPCODE(mnemonic, value) \
  case Opcode::mnemonic:        \
    return #mnemonic;
#include <cpu_opcodes.def>
#undef OPCODE
    default: return "UNKNOWN";
  }
}

}  // namespace cpu
