#pragma once

#include <types.hpp>

#include <array>

namespace bus {
class Bus;
}

namespace cpu {

using Register = u32;

class Cpu {
 public:
  explicit Cpu(bus::Bus const& bus);
  bool step(u32& cycles_passed);

 private:
  bus::Bus const& m_bus;

  Register m_pc{};
  std::array<Register, 32> m_gpr{};
};

}  // namespace cpu
