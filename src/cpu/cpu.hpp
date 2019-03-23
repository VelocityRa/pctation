#pragma once

#include <util/types.hpp>

#include <gsl-lite.hpp>

#include <array>

namespace bus {
class Bus;
}

namespace cpu {
class Instruction;
}

namespace cpu {
constexpr auto PC_RESET_ADDR = 0xBFC00000u;

using Register = u32;

class Cpu {
 public:
  explicit Cpu(bus::Bus const& bus);
  bool step(u32& cycles_passed);
  void execute_instruction(const Instruction& i);

// Register getters
private:
 Register& r(u8 index) {
    Ensures(index >= 0);
    Ensures(index <= 32);
    return m_gpr[index];
  }
 Register &pc()
 {
  return m_pc;
 }

 private:
  bus::Bus const& m_bus;

  Register m_pc{ PC_RESET_ADDR };
  std::array<Register, 32> m_gpr{};
};

}  // namespace cpu
