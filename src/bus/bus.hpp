#pragma once

#include <util/types.hpp>

namespace bios {
class Bios;
}

namespace cpu {
class Interrupts;
}

namespace memory {
class Ram;
class Scratchpad;
class Dma;
class Expansion;
}  // namespace memory

namespace gpu {
class Gpu;
}

namespace spu {
class Spu;
}

namespace io {
class Joypad;
class CdromDrive;
class Timers;
}  // namespace io

namespace bus {

class Bus {
 public:
  explicit Bus(bios::Bios const& bios,
               memory::Expansion& expansion,
               cpu::Interrupts& interrupts,
               memory::Scratchpad& scratchpad,
               memory::Ram& ram,
               memory::Dma& dma,
               gpu::Gpu& gpu,
               spu::Spu& spu,
               io::Joypad& joypad,
               io::CdromDrive& cdrom,
               io::Timers& timers)

      : m_interrupts(interrupts),
        m_ram(ram),
        m_expansion(expansion),
        m_scratchpad(scratchpad),
        m_bios(bios),
        m_dma(dma),
        m_gpu(gpu),
        m_spu(spu),
        m_joypad(joypad),
        m_cdrom(cdrom),
        m_timers(timers) {}

  u32 read32(u32 addr) const;
  u16 read16(u32 addr) const;
  u8 read8(u32 addr) const;
  void write32(u32 addr, u32 val);
  void write16(u32 addr, u16 val);
  void write8(u32 addr, u8 val);

  cpu::Interrupts& m_interrupts;
  memory::Ram& m_ram;

 private:
  memory::Expansion& m_expansion;
  memory::Scratchpad& m_scratchpad;
  bios::Bios const& m_bios;
  memory::Dma& m_dma;
  gpu::Gpu& m_gpu;
  spu::Spu& m_spu;
  io::Joypad& m_joypad;
  io::CdromDrive& m_cdrom;
  io::Timers& m_timers;
};

}  // namespace bus
