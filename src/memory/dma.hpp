#pragma once

#include <memory/dma_channel.hpp>
#include <util/log.hpp>
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

  template <typename ValueType>
  ValueType read(address addr_rebased) const {
    const auto major = (addr_rebased & 0x70) >> 4;
    const auto minor = addr_rebased & 0b1100;

    const u32* reg = nullptr;

    if (0 <= major && major <= 6) {
      // Per-channel registers
      const auto& channel = channel_control(static_cast<DmaPort>(major));

      switch (minor) {
        case 0: reg = &channel.m_base_addr; break;
        case 4: reg = &channel.m_block_control.word; break;
        case 8: reg = &channel.m_channel_control.word; break;
        default: LOG_WARN("Unhandled read from DMA at offset 0x{:08X}", addr_rebased); return 0;
      }
    } else if (major == 7) {
      // Common registers
      switch (minor) {
        case 0: reg = &m_reg_control; break;
        case 4: reg = &m_reg_interrupt.word; break;
        default: LOG_WARN("Unhandled read from DMA at offset 0x{:08X}", addr_rebased); break;
      }
    } else {
      LOG_WARN("Unhandled read from DMA at offset 0x{:08X}", addr_rebased);
      return 0;
    }

    // Do the read
    const u32 reg_offset = addr_rebased & 0b11;
    return *(ValueType*)((u8*)reg + reg_offset);
  }

  template <typename ValueType>
  void write(address addr_rebased, ValueType val) {
    const auto major = (addr_rebased & 0x70) >> 4;
    const auto minor = addr_rebased & 0b1100;

    const u32* reg = nullptr;
    DmaChannel* channel = nullptr;

    if (0 <= major && major <= 6) {
      // Per-channel registers
      channel = &channel_control(static_cast<DmaPort>(major));

      switch (minor) {
        case 0: reg = &channel->m_base_addr; break;
        case 4: reg = &channel->m_block_control.word; break;
        case 8: reg = &channel->m_channel_control.word; break;
        default:
          LOG_WARN("Unhandled write to DMA register: 0x{:08X} at offset 0x{:08X}", val, addr_rebased);
          return;
      }
    } else if (major == 7) {
      // Common registers
      switch (minor) {
        case 0: reg = &m_reg_control; break;
        case 4: {
          // Handle u32 case separately because we need to do masking

          if (std::is_same<ValueType, u8>::value || std::is_same<ValueType, u16>::value) {
            reg = &m_reg_interrupt.word;
            break;
          } else if (std::is_same<ValueType, u32>::value) {
            // Clear acknowledged (1'ed) flag bits
            u32 masked_flags = (((m_reg_interrupt.word & 0xFF000000) & ~(val & 0xFF000000)));
            m_reg_interrupt.word = (val & 0x00FFFFFF) | masked_flags;
            return;  // write handled here, we can return instead of breaking
          } else
            static_assert("16-bit write unsuported");
        }
        default:
          LOG_WARN("Unhandled write to DMA register: 0x{:08X} at offset 0x{:08X}", val, addr_rebased);
          return;
      }
    } else {
      LOG_WARN("Unhandled write to DMA register: 0x{:08X} at offset 0x{:08X}", val, addr_rebased);
      return;
    }

    // Do the write
    const u32 reg_offset = addr_rebased & 0b11;
    *(ValueType*)((u8*)reg + reg_offset) = val;

    // Handle activated transfers
    if (channel && channel->active()) {
      const auto port = static_cast<DmaPort>(major);
      do_transfer(port);
    }
  }

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
