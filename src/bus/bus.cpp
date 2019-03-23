#include <bios/bios.hpp>
#include <bus/bus.hpp>
#include <memory/map.hpp>
#include <util/log.hpp>

#include <gsl-lite.hpp>

namespace bus {

u32 Bus::read32(u32 addr) const {
  // Only whole words are addressable
  Expects(addr % 4 == 0);

  address addr_rebased;

  if (memory::map::BIOS.contains(addr, addr_rebased))
    return m_bios.read32(addr_rebased);

  LOG::error("Unknown memory read at 0x{:08X}", addr);
  return 0;
}

}  // namespace bus
