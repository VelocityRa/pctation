#include <bios.hpp>
#include <bus.hpp>

#include <cassert>

namespace bus {

u32 Bus::read32(u32 addr) const {
  // Only whole words are addressable
  assert(addr % 4 == 0);

  return m_bios.read32(addr);
}

}  // namespace bus
