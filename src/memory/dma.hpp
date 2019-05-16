#pragma once

#include <memory/dma_channel.hpp>
#include <util/types.hpp>

#include <array>
#include <bitset>

namespace memory {
class Ram;
}

namespace gpu {
class Gpu;
}

namespace cpu {
class Interrupts;
}

namespace io {
class CdromDrive;
}

namespace memory {

enum class DmaPort {
  MdecIn = 0,   // Macroblock decoder input
  MdecOut = 1,  // Macroblock decoder output
  Gpu = 2,      // Graphics Processing Unit
  Cdrom = 3,    // CD-ROM drive
  Spu = 4,      // Sound Processing Unit
  Pio = 5,      // Extension port
  Otc = 6,      // Used to clear the ordering table
};

static const char* dma_port_to_str(DmaPort dma_port) {
  switch (dma_port) {
    case DmaPort::MdecIn: return "MDECin";
    case DmaPort::MdecOut: return "MDECout";
    case DmaPort::Gpu: return "GPU";
    case DmaPort::Cdrom: return "CD-ROM";
    case DmaPort::Spu: return "SPU";
    case DmaPort::Pio: return "PIO";
    case DmaPort::Otc: return "OTC";
    default: return "<Invalid>";
  }
}

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

  bool is_port_enabled(DmaPort port) const { return (word & (1 << (16 + (u8)port))) || master_enable; }
  void set_port_flags(DmaPort port, bool val) {
    std::bitset<32> bs(word);
    bs.set((u8)port + 24, val);
    word = bs.to_ulong();
  }

  bool get_irq_master_flag() {
    u8 all_enable = (word & 0x7F0000) >> 16;
    u8 all_flag = (word & 0x7F000000) >> 24;
    return force || (master_enable && (all_enable & all_flag));
  }
};

class Dma {
 public:
  explicit Dma(memory::Ram& ram, gpu::Gpu& gpu, cpu::Interrupts& interrupts, io::CdromDrive& cdrom)
      : m_ram(ram),
        m_gpu(gpu),
        m_interrupts(interrupts),
        m_cdrom(cdrom) {}

  void write_reg(address addr, u32 val);
  u32 read_reg(address addr) const;
  DmaChannel const& channel_control(DmaPort port) const;
  DmaChannel& channel_control(DmaPort port);
  void step();

 private:
  void do_transfer(DmaPort port);
  void do_block_transfer(DmaPort port);
  void transfer_finished(DmaChannel& channel, DmaPort port);
  void do_linked_list_transfer(DmaPort port);

 private:
  u32 m_reg_control{ 0x07654321 };
  DmaInterruptRegister m_reg_interrupt;
  bool m_irq_pending{};

  std::array<DmaChannel, 7> m_channels{};

  memory::Ram& m_ram;
  gpu::Gpu& m_gpu;
  cpu::Interrupts& m_interrupts;
  io::CdromDrive& m_cdrom;
};

}  // namespace memory
