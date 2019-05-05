#pragma once

#include <memory/dma_channel.hpp>
#include <util/types.hpp>

#include <array>

namespace memory {
class Ram;
}

namespace gpu {
class Gpu;
}

namespace memory {

enum class DmaPort {
  MdecIn = 0,   // Macroblock decoder input
  MdecOut = 1,  // Macroblock decoder output
  Gpu = 2,      // Graphics Processing Unit
  CdRom = 3,    // CD-ROM drive
  Spu = 4,      // Sound Processing Unit
  Pio = 5,      // Extension port
  Otc = 6,      // Used to clear the ordering table
};

static const char* dma_port_to_str(DmaPort dma_port) {
  switch (dma_port) {
    case DmaPort::MdecIn: return "MDECin";
    case DmaPort::MdecOut: return "MDECout";
    case DmaPort::Gpu: return "GPU";
    case DmaPort::CdRom: return "CD-ROM";
    case DmaPort::Spu: return "SPU";
    case DmaPort::Pio: return "PIO";
    case DmaPort::Otc: return "OTC";
    default: return "<Invalid>";
  }
}

enum class DmaRegister : u32 {
  DMA_GPU_CONTROL = 0x28,
  DMA_CONTROL = 0x70,
  DMA_INTERRUPT = 0x74,
};

union DmaInterruptRegister {
  u32 word{};

  struct {
    u32 _0_14 : 15;
    u32 force : 1;

    u32 dec_in_enable : 1;
    u32 dec_out_enable : 1;
    u32 gpu_enable : 1;
    u32 cdrom_enable : 1;
    u32 spu_enable : 1;
    u32 ext_enable : 1;
    u32 ram_enable : 1;
    u32 master_enable : 1;

    u32 dec_in_flags : 1;
    u32 dec_out_flags : 1;
    u32 gpu_flags : 1;
    u32 cdrom_flags : 1;
    u32 spu_flags : 1;
    u32 ext_flags : 1;
    u32 ram_flags : 1;
    u32 master_flags : 1;
  };
};

class Dma {
 public:
  explicit Dma(memory::Ram& ram, gpu::Gpu& gpu) : m_ram(ram), m_gpu(gpu) {}

  void set_reg(DmaRegister reg, u32 val);
  u32 read_reg(DmaRegister reg) const;
  DmaChannel const& channel_control(DmaPort port) const;
  DmaChannel& channel_control(DmaPort port);

 private:
  void do_transfer(DmaPort port);
  void do_block_transfer(DmaPort port);
  void do_linked_list_transfer(DmaPort port);

 private:
  DmaInterruptRegister m_interrupt;
  std::array<DmaChannel, 7> m_channels{};
  u32 m_control{ 0x07654321 };

  memory::Ram& m_ram;
  gpu::Gpu& m_gpu;
};

}  // namespace memory
