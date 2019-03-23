#pragma once

#include <types.hpp>

namespace bios {
class Bios;
}

namespace bus {

class Bus {
 public:
  explicit Bus(bios::Bios const& bios) : m_bios(bios) {}
  u32 read32(u32 addr) const;

 private:
  bios::Bios const& m_bios;
};

}  // namespace bus
