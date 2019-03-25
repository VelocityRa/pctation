#pragma once

#include <util/types.hpp>

namespace bios {
class Bios;
}
namespace memory {
class Ram;
}

namespace bus {

class Bus {
 public:
  explicit Bus(bios::Bios const& bios, memory::Ram& ram) : m_bios(bios), m_ram(ram) {}
  u32 read32(u32 addr) const;
  void write32(u32 addr, u32 val);

 private:
  bios::Bios const& m_bios;
  memory::Ram& m_ram;
};

}  // namespace bus
