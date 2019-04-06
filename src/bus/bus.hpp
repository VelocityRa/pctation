#pragma once

#include <util/types.hpp>

namespace bios {
class Bios;
}
namespace memory {
class Ram;
class Dma;
}  // namespace memory

namespace gpu {
class Gpu;
}

namespace bus {

class Bus {
 public:
  explicit Bus(bios::Bios const& bios, memory::Ram& ram, memory::Dma& dma, gpu::Gpu& gpu)
      : m_bios(bios),
        m_ram(ram),
        m_dma(dma),
        m_gpu(gpu) {}
  u32 read32(u32 addr) const;
  u16 read16(u32 addr) const;
  u8 read8(u32 addr) const;
  void write32(u32 addr, u32 val) const;
  void write16(u32 addr, u16 val) const;
  void write8(u32 addr, u8 val) const;

 private:
  bios::Bios const& m_bios;
  memory::Ram& m_ram;
  memory::Dma& m_dma;
  gpu::Gpu& m_gpu;
};

}  // namespace bus
