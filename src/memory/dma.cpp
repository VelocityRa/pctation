#include <memory/dma.hpp>

#include <util/log.hpp>

namespace memory {

void Dma::set_reg(DmaRegister reg, u32 val) {
  switch (reg) {
    case DmaRegister::DMA_GPU_CONTROL: m_gpu_control.word = val; break;
    case DmaRegister::DMA_CONTROL: m_control = val; break;
    case DmaRegister::DMA_INTERRUPT: m_irq.word = val; break;
    default: LOG_WARN("Unhandled write to DMA register: 0x{:08X} at 0x{:08X}", val, (u32)reg); break;
  }
}

u32 Dma::read_reg(DmaRegister reg) const {
  switch (reg) {
    case DmaRegister::DMA_GPU_CONTROL: return m_gpu_control.word;
    case DmaRegister::DMA_CONTROL: return m_control;
    case DmaRegister::DMA_INTERRUPT: return m_irq.word;
    default: LOG_WARN("Unhandled read from DMA register at 0x{:08X}", (u32)reg);
  }
  return 0;
}

}  // namespace memory
