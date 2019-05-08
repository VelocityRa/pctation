#pragma once

#include <util/types.hpp>

namespace cpu {
class Interrupts;
}

namespace io {

class Cdrom {
 public:
  void init(cpu::Interrupts* interrupts);
  u8 read_reg(address addr_rebased);
  void write_reg(address addr_rebased, u8 val);

 private:
  u8 m_index{};

  cpu::Interrupts* m_interrupts{};
};

}  // namespace io