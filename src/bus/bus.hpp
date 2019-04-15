#pragma once

#include <util/types.hpp>

namespace bios {
class Bios;
}
namespace memory {
class Ram;
class Scratchpad;
class Dma;
}  // namespace memory

namespace gpu {
class Gpu;
}

namespace bus {

class Bus {
 public:
  explicit Bus(bios::Bios const& bios,
               memory::Scratchpad& scratchpad,
               memory::Ram& ram,
               memory::Dma& dma,
               gpu::Gpu& gpu)
      : m_ram(ram),
        m_scratchpad(scratchpad),
        m_bios(bios),
        m_dma(dma),
        m_gpu(gpu) {}
  u32 read32(u32 addr) const;
  u16 read16(u32 addr) const;
  u8 read8(u32 addr) const;
  void write32(u32 addr, u32 val) const;
  void write16(u32 addr, u16 val) const;
  void write8(u32 addr, u8 val) const;

  memory::Ram& m_ram;

 private:
  memory::Scratchpad& m_scratchpad;
  bios::Bios const& m_bios;
  memory::Dma& m_dma;
  gpu::Gpu& m_gpu;
};

}  // namespace bus
