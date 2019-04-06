#include <bios/bios.hpp>
#include <bus/bus.hpp>
#include <gpu/gpu.hpp>
#include <memory/dma.hpp>
#include <memory/map.hpp>
#include <memory/ram.hpp>
#include <util/log.hpp>

namespace bus {

u32 Bus::read32(u32 addr) const {
  addr = memory::mask_region(addr);

  address addr_rebased;

  if (memory::map::RAM.contains(addr, addr_rebased))
    return m_ram.read32(addr_rebased);
  if (memory::map::IRQ_CONTROL.contains(addr, addr_rebased)) {
    LOG_WARN("Unhandled 32-bit read of {}", addr_rebased == 0 ? "I_STAT" : "I_MASK");
    return 0;
  }
  if (memory::map::BIOS.contains(addr, addr_rebased))
    return m_bios.read32(addr_rebased);
  if (memory::map::DMA.contains(addr, addr_rebased))
    return m_dma.read_reg(static_cast<memory::DmaRegister>(addr_rebased));
  if (memory::map::GPU.contains(addr, addr_rebased))
    return m_gpu.read32(addr_rebased);
  if (memory::map::TIMERS.contains(addr, addr_rebased)) {
    LOG_WARN("Unhandled 32-bit read of Timer register at 0x{:08X}", addr);
    return 0;
  }

  LOG_ERROR("Unknown 32-bit read at 0x{:08X}", addr);
  assert(0);
  return 0;
}

u16 Bus::read16(u32 addr) const {
  addr = memory::mask_region(addr);

  address addr_rebased;

  if (memory::map::RAM.contains(addr, addr_rebased))
    return m_ram.read16(addr_rebased);
  if (memory::map::SPU.contains(addr, addr_rebased)) {
    // NOTE: TRACE level because it's used a lot in BIOS init.
    LOG_TRACE("Unhandled 16-bit read of SPU register at 0x{:08X}", addr);
    return 0;
  }
  if (memory::map::IRQ_CONTROL.contains(addr, addr_rebased)) {
    LOG_WARN("Unhandled 16-bit read of {}", addr_rebased == 0 ? "I_STAT" : "I_MASK");
    return 0;
  }

  LOG_ERROR("Unknown 16-bit read at 0x{:08X}", addr);
  assert(0);
  return 0;
}

u8 Bus::read8(u32 addr) const {
  addr = memory::mask_region(addr);

  address addr_rebased;

  if (memory::map::RAM.contains(addr, addr_rebased))
    return m_ram.read8(addr_rebased);
  if (memory::map::BIOS.contains(addr, addr_rebased))
    return m_bios.read8(addr_rebased);
  if (memory::map::EXPANSION_1.contains(addr, addr_rebased)) {
    LOG_WARN("Unhandled 8-bit read of EXPANSION 1 memory at 0x{:08X}", addr);
    // No expansion unimplemented
    return 0xFF;
  }

  LOG_ERROR("Unknown 8-bit read at 0x{:08X} ", addr);
  assert(0);
  return 0;
}

void Bus::write32(u32 addr, u32 val) const {
  addr = memory::mask_region(addr);

  address addr_rebased;

  if (memory::map::RAM.contains(addr, addr_rebased))
    return m_ram.write32(addr_rebased, val);
  if (memory::map::IRQ_CONTROL.contains(addr, addr_rebased)) {
    LOG_WARN("Unhandled 32-bit write of 0x{:08X} to {}", val, addr_rebased == 0 ? "I_STAT" : "I_MASK");
    return;
  }
  if (memory::map::MEM_CONTROL1.contains(addr, addr_rebased)) {
    switch (addr_rebased) {
      case 0x0:
        if (val != 0x1F000000)
          LOG_CRITICAL("Unhandled EXPANSION_1 base address: 0x{:08X}", val);
        return;
      case 0x4:
        if (val != 0x1F802000)
          LOG_CRITICAL("Unhandled EXPANSION_2 base address: 0x{:08X}", val);
        return;
      case 0x8:   // Expansion 1 Delay/Size
      case 0xC:   // Expansion 3 Delay/Size
      case 0x10:  // BIOS ROM Delay/Size
      case 0x14:  // SPU_DELAY Delay/Size
      case 0x18:  // CDROM_DELAY Delay/Size
      case 0x1C:  // Expansion 2 Delay/Size
      case 0x20:  // COM_DELAY
        if (val != 0)
          LOG_WARN("Unhandled non-0 32-bit write to MEM_CONTROL1: 0x{:08X} at 0x{:08X}", val, addr);
        return;
      default:
        LOG_WARN("Unhandled 32-bit write to MEM_CONTROL1: 0x{:08X} at 0x{:08X}", val, addr);
        return;
    }
  }
  if (memory::map::MEM_CONTROL2.contains(addr, addr_rebased)) {
    return;  // RAM_SIZE, ignore
  }
  if (memory::map::MEM_CONTROL3.contains(addr, addr_rebased)) {
    return;  // Cache Control, ignore
  }
  if (memory::map::DMA.contains(addr, addr_rebased))
    return m_dma.set_reg(static_cast<memory::DmaRegister>(addr_rebased), val);
  if (memory::map::GPU.contains(addr, addr_rebased))
    return m_gpu.write32(addr_rebased, val);
  if (memory::map::TIMERS.contains(addr, addr_rebased)) {
    LOG_WARN("Unhandled 32-bit write to Timer register: 0x{:08X} at 0x{:08X}", val, addr);
    return;
  }

  LOG_ERROR("Unknown 32-bit write of 0x{:08X} at 0x{:08X} ", val, addr);
  assert(0);
}

void Bus::write16(u32 addr, u16 val) const {
  addr = memory::mask_region(addr);

  address addr_rebased;

  if (memory::map::RAM.contains(addr, addr_rebased))
    return m_ram.write16(addr_rebased, val);
  if (memory::map::TIMERS.contains(addr, addr_rebased)) {
    LOG_WARN("Unhandled 16-bit write to Timer register: 0x{:04X} at 0x{:08X}", val, addr);
    return;
  }
  if (memory::map::SPU.contains(addr, addr_rebased)) {
    // NOTE: TRACE level because it's used a lot in BIOS init.
    LOG_TRACE("Unhandled 16-bit write to SPU register: 0x{:04X} at 0x{:08X}", val, addr);
    return;
  }
  if (memory::map::IRQ_CONTROL.contains(addr, addr_rebased)) {
    LOG_WARN("Unhandled 16-bit write of 0x{:04X} to {}", val, addr_rebased == 0 ? "I_STAT" : "I_MASK");
    return;
  }

  LOG_ERROR("Unknown 16-bit write of 0x{:04X} at 0x{:08X} ", val, addr);
  assert(0);
}

void Bus::write8(u32 addr, u8 val) const {
  addr = memory::mask_region(addr);

  address addr_rebased;

  if (memory::map::RAM.contains(addr, addr_rebased))
    return m_ram.write8(addr_rebased, val);
  if (memory::map::EXPANSION_2.contains(addr, addr_rebased)) {
    LOG_WARN("Unhandled 8-bit write to EXPANSION_2 register: 0x{:02X} at 0x{:08X}", val, addr);
    return;
  }

  LOG_ERROR("Unknown 8-bit write of 0x{:02X} at 0x{:08X} ", val, addr);
  assert(0);
}

}  // namespace bus
