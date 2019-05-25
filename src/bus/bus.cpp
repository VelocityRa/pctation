#include <bios/bios.hpp>
#include <bus/bus.hpp>
#include <cpu/interrupt.hpp>
#include <gpu/gpu.hpp>
#include <io/cdrom_drive.hpp>
#include <io/joypad.hpp>
#include <io/timers.hpp>
#include <memory/dma.hpp>
#include <memory/expansion.hpp>
#include <memory/map.hpp>
#include <memory/ram.hpp>
#include <spu/spu.hpp>
#include <util/log.hpp>

namespace bus {

// TODO: refactor IO reads/writes

u32 Bus::read32(u32 addr) const {
  addr = memory::mask_region(addr) & 0x1FFFFFFC;

  address addr_rebased;

  if (memory::map::RAM.contains(addr, addr_rebased))
    return m_ram.read<u32>(addr_rebased);
  if (memory::map::SCRATCHPAD.contains(addr, addr_rebased))
    return m_scratchpad.read<u32>(addr_rebased);
  if (memory::map::IRQ_CONTROL.contains(addr, addr_rebased)) {
    auto val = m_interrupts.read<u32>(addr_rebased);
    LOG_DEBUG("{} 32-bit read of 0x{:08X}", addr_rebased == 0 ? "I_STAT" : "I_MASK", val);
    return val;
  }
  if (memory::map::BIOS.contains(addr, addr_rebased))
    return m_bios.read<u32>(addr_rebased);
  if (memory::map::DMA.contains(addr, addr_rebased))
    return m_dma.read<u32>(addr_rebased);
  if (memory::map::GPU.contains(addr, addr_rebased))
    return m_gpu.read_reg(addr_rebased);
  if (memory::map::TIMERS.contains(addr, addr_rebased))
    return static_cast<u32>(m_timers.read_reg(addr_rebased));
  }
  if (memory::map::EXPANSION_1.contains(addr, addr_rebased)) {
    if (addr_rebased == 0x020018)
      __debugbreak();
    return m_expansion.read<u32>(addr_rebased);
  }

  LOG_ERROR("Unknown 32-bit read at 0x{:08X}", addr);
  assert(0);
  return 0;
}

u16 Bus::read16(u32 addr) const {
  addr = memory::mask_region(addr) & 0x1FFFFFFE;

  address addr_rebased;

  if (memory::map::RAM.contains(addr, addr_rebased))
    return m_ram.read<u16>(addr_rebased);
  if (memory::map::SCRATCHPAD.contains(addr, addr_rebased))
    return m_scratchpad.read<u16>(addr_rebased);
  if (memory::map::SPU.contains(addr, addr_rebased)) {
    // NOTE: TRACE level because it's used a lot in BIOS init.
    LOG_TRACE("Stubbed 16-bit read of SPU register at 0x{:08X}", addr);
    return m_spu.read<u16>(addr_rebased);
  }
  if (memory::map::IRQ_CONTROL.contains(addr, addr_rebased)) {
    auto val = m_interrupts.read<u16>(addr_rebased);
    LOG_DEBUG("{} 16-bit read of 0x{:04X}", addr_rebased == 0 ? "I_STAT" : "I_MASK", val);
    return val;
  }
  if (memory::map::JOYPAD.contains(addr, addr_rebased)) {
    auto val = (u16)m_joypad.read8(addr_rebased) | (u16)m_joypad.read8(addr_rebased + 1) << 8;
    LOG_TRACE("{} 16-bit read of 0x{:04X}", io::Joypad::addr_to_reg_name(addr_rebased), val);
    return val;
  }
  if (memory::map::EXPANSION_1.contains(addr, addr_rebased)) {
    if (addr_rebased == 0x020018)
      __debugbreak();
    return m_expansion.read<u16>(addr_rebased);
  }
  if (memory::map::SIO.contains(addr, addr_rebased)) {
    LOG_WARN("Unhandled 16-bit read of SIO register at 0x{:04X}", addr);
    return 0;
  }
  if (memory::map::TIMERS.contains(addr, addr_rebased))
    return m_timers.read_reg(addr_rebased);

  LOG_ERROR("Unknown 16-bit read at 0x{:08X}", addr);
  assert(0);
  return 0;
}

u8 Bus::read8(u32 addr) const {
  addr = memory::mask_region(addr);

  address addr_rebased;

  if (memory::map::RAM.contains(addr, addr_rebased))
    return m_ram.read<u8>(addr_rebased);
  if (memory::map::SCRATCHPAD.contains(addr, addr_rebased))
    return m_scratchpad.read<u8>(addr_rebased);
  if (memory::map::BIOS.contains(addr, addr_rebased))
    return m_bios.read<u8>(addr_rebased);
  if (memory::map::JOYPAD.contains(addr, addr_rebased)) {
    auto val = m_joypad.read8(addr_rebased);
    LOG_TRACE("{} 8-bit read of 0x{:02X}", io::Joypad::addr_to_reg_name(addr_rebased), val);
    return val;
  }
  if (memory::map::EXPANSION_1.contains(addr, addr_rebased)) {
    if (addr_rebased == 0x020018)
      __debugbreak();
    return m_expansion.read<u8>(addr_rebased);
  }
  if (memory::map::EXPANSION_2.contains(addr, addr_rebased)) {
    LOG_WARN("Unhandled 8-bit read of EXPANSION_2 register at 0x{:08X}", addr);
    return 0;
  }
  if (memory::map::CDROM.contains(addr, addr_rebased))
    return m_cdrom.read_reg(addr_rebased);
  if (memory::map::DMA.contains(addr, addr_rebased))
    return m_dma.read<u8>(addr_rebased);
  if (memory::map::SIO.contains(addr, addr_rebased)) {
    LOG_WARN("Unhandled 8-bit read of SIO register at 0x{:08X}", addr);
    return 0;
  }

  LOG_ERROR("Unknown 8-bit read at 0x{:08X} ", addr);
  assert(0);
  return 0;
}

void Bus::write32(u32 addr, u32 val) {
  addr = memory::mask_region(addr) & 0x1FFFFFFC;

  address addr_rebased;

  if (memory::map::RAM.contains(addr, addr_rebased))
    return m_ram.write<u32>(addr_rebased, val);
  if (memory::map::SCRATCHPAD.contains(addr, addr_rebased))
    return m_scratchpad.write<u32>(addr_rebased, val);
  if (memory::map::SPU.contains(addr, addr_rebased)) {
    LOG_WARN("Stubbed 32-bit write to SPU register: 0x{:08X} at 0x{:08X}", val, addr);
    return m_spu.write<u32>(addr_rebased, val);
  }
  if (memory::map::IRQ_CONTROL.contains(addr, addr_rebased)) {
    LOG_DEBUG("{} 32-bit write of 0x{:08X}", addr_rebased == 0 ? "I_STAT" : "I_MASK", val);
    return m_interrupts.write<u32>(addr_rebased, val);
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
    return m_dma.write<u32>(addr_rebased, val);
  if (memory::map::GPU.contains(addr, addr_rebased))
    return m_gpu.write_reg(addr_rebased, val);
  if (memory::map::TIMERS.contains(addr, addr_rebased)) {
    m_timers.write_reg(addr_rebased, static_cast<u16>(val));
    return;
  }

  LOG_ERROR("Unknown 32-bit write of 0x{:08X} at 0x{:08X} ", val, addr);
  assert(0);
}

void Bus::write16(u32 addr, u16 val) {
  addr = memory::mask_region(addr) & 0x1FFFFFFE;

  address addr_rebased;

  if (memory::map::RAM.contains(addr, addr_rebased))
    return m_ram.write<u16>(addr_rebased, val);
  if (memory::map::SCRATCHPAD.contains(addr, addr_rebased))
    return m_scratchpad.write<u16>(addr_rebased, val);
  if (memory::map::TIMERS.contains(addr, addr_rebased)) {
    m_timers.write_reg(addr_rebased, val);
    return;
  }
  if (memory::map::SPU.contains(addr, addr_rebased)) {
    // NOTE: TRACE level because it's used a lot in BIOS init.
    LOG_TRACE("Stubbed 16-bit write to SPU register: 0x{:04X} at 0x{:08X}", val, addr);
    return m_spu.write<u16>(addr_rebased, val);
  }
  if (memory::map::IRQ_CONTROL.contains(addr, addr_rebased)) {
    LOG_DEBUG("{} 16-bit write of 0x{:04X}", addr_rebased == 0 ? "I_STAT" : "I_MASK", val);
    return m_interrupts.write<u16>(addr_rebased, val);
  }
  if (memory::map::JOYPAD.contains(addr, addr_rebased)) {
    LOG_TRACE("16-bit write of {:04X} to {}", val, io::Joypad::addr_to_reg_name(addr_rebased));
    m_joypad.write8(addr_rebased, val & 0xFF);
    m_joypad.write8(addr_rebased + 1, (val >> 8) & 0xFF);
    return;
  }
  if (memory::map::SIO.contains(addr, addr_rebased)) {
    LOG_WARN("Unhandled 16-bit write to SIO register: 0x{:04X} at 0x{:08X}", val, addr);
    return;
  }

  LOG_ERROR("Unknown 16-bit write of 0x{:04X} at 0x{:08X} ", val, addr);
  assert(0);
}

void Bus::write8(u32 addr, u8 val) {
  addr = memory::mask_region(addr);

  address addr_rebased;

  if (memory::map::RAM.contains(addr, addr_rebased))
    return m_ram.write<u8>(addr_rebased, val);
  if (memory::map::SCRATCHPAD.contains(addr, addr_rebased))
    return m_scratchpad.write<u8>(addr_rebased, val);
  if (memory::map::JOYPAD.contains(addr, addr_rebased)) {
    LOG_TRACE("8-bit write of {:02X} to {}", val, io::Joypad::addr_to_reg_name(addr_rebased));
    m_joypad.write8(addr_rebased, val);
    return;
  }
  if (memory::map::EXPANSION_2.contains(addr, addr_rebased)) {
    LOG_WARN("Unhandled 8-bit write to EXPANSION_2 register: 0x{:02X} at 0x{:08X}", val, addr);
    return;
  }
  if (memory::map::CDROM.contains(addr, addr_rebased)) {
    m_cdrom.write_reg(addr_rebased, val);
    return;
  }
  if (memory::map::DMA.contains(addr, addr_rebased))
    return m_dma.write<u8>(addr_rebased, val);

  LOG_ERROR("Unknown 8-bit write of 0x{:02X} at 0x{:08X} ", val, addr);
  assert(0);
}

}  // namespace bus
