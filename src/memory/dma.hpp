#pragma once

#include <util/types.hpp>

namespace memory {

enum class DmaRegister : u32 {
  DMA_GPU_CONTROL = 0x28,
  DMA_CONTROL = 0x70,
  DMA_INTERRUPT = 0x74,
};

class Dma {
 public:
  void set_reg(DmaRegister reg, u32 val);
  u32 read_reg(DmaRegister reg) const;

 private:
  union {
    u32 word;

    struct {
      u32 _0_14 : 15;  // [14:0] unknown/unused
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
  } m_irq;

  union {
    u32 word;

    struct {
      u32 from_ram : 1;  // Transfer direction (0: to RAM, 1: from RAM)
      u32 memory_address_step : 1;
      u32 _2_7 : 6;
      u32 chopping_enable : 1;
      u32 sync_mode : 2;
      u32 _11_15 : 5;
      u32 chopping_dma_window_size : 3;
      u32 _19 : 1;
      u32 chopping_cpu_window_size : 3;
      u32 _23 : 1;
      u32 enable : 1;
      u32 _25_27 : 3;
      u32 manual_trigger : 1;
      u32 _29_31 : 3;
    };
  } m_gpu_control;

  u32 m_control{ 0x07654321 };
};

}  // namespace memory
