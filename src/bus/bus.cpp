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

  LOG_ERROR("Unknown memory read at 0x{:08X}", addr);
  assert(0);
  return 0;
}

void Bus::write32(u32 addr, u32 val) {
  // Only whole words are addressable
  Expects(addr % 4 == 0);

  address addr_rebased;

  if (memory::map::MEM_CONTROL1.contains(addr, addr_rebased)) {
    switch (addr_rebased) {
      case 0x0:
        if (val != 0x1F000000)
          LOG_CRITICAL("Unhandled expansion 1 base address:  0x{:08X}", val);
        return;
      case 0x4:
        if (val != 0x1F802000)
          LOG_CRITICAL("Unhandled expansion 2 base address:  0x{:08X}", val);
        return;
      case 0x8: return;   // Expansion 1 Delay/Size, ignore
      case 0xC: return;   // Expansion 3 Delay/Size, ignore
      case 0x10: return;  // BIOS ROM Delay/Size, ignore
      case 0x14: return;  // SPU_DELAY Delay/Size, ignore
      case 0x18: return;  // CDROM_DELAY Delay/Size, ignore
      case 0x1C: return;  // Expansion 2 Delay/Size, ignore
      case 0x20: return;  // COM_DELAY, ignore
      default:
        LOG_WARN("Unhandled write to MEM_CONTROL1: 0x{:08X} at 0x{:08X}", val, addr_rebased);
        return;
    }
  } else if (memory::map::MEM_CONTROL2.contains(addr, addr_rebased)) {
    return;  // RAM_SIZE, ignore
  } else if (memory::map::MEM_CONTROL3.contains(addr, addr_rebased)) {
    return;  // Cache Control, ignore
  }

  LOG_ERROR("Unknown memory write of 0x{:08X} at 0x{:08X} ", val, addr);
  assert(0);
}

}  // namespace bus
