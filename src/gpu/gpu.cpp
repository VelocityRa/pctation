#include <gpu/gpu.hpp>
#include "util/log.hpp"

namespace gpu {

u32 Gpu::read32(u32 addr) {
  switch (addr) {
    case 0: LOG_WARN("Unhandled 32-bit read of GPUREAD"); return 0;
    case 4: return gpustat();
  }
  // Unreachable because the GPU Range only includes the above 2 registers
  return 0;
}

void Gpu::write32(u32 addr, u32 val) {
  switch (addr) {
    case 0: gp0(val); break;
    case 4: gp1(val); break;
  }
}

void Gpu::gp0(u32 cmd) {
  const auto opcode = (cmd >> 24) & 0xFF;
  const auto args = (cmd & 0xFFFFFF) >> 8;

  LOG_DEBUG("GP0 cmd: {:08X}", cmd);

  switch (opcode) {
    case 0x00: break;
    case 0xE1: gp0_draw_mode(cmd); break;
    case 0xE2: m_tex_window.word = cmd; break;
    case 0xE3: m_drawing_area_top_left.word = cmd; break;
    case 0xE4: m_drawing_area_bottom_right.word = cmd; break;
    case 0xE5: m_drawing_offset.word = cmd; break;
    case 0xE6: gp0_mask_bit(cmd); break;
    case 0x1F: gp1_gpu_irq(); break;
    default: LOG_WARN("Unhandled GP0 cmd: 0x{:08X}", cmd); break;
  }
}

void Gpu::gp0_draw_mode(u32 cmd) {
  const u32 draw_mode_to_gpustat_mask = 0b11111111111u;

  // GPUSTAT.0-10 = GP0(E1).0-10
  m_gpustat.word &= ~draw_mode_to_gpustat_mask;
  m_gpustat.word |= cmd & draw_mode_to_gpustat_mask;

  // GPUSTAT.15 = GP0(E1).11
  m_gpustat.tex_disable = (cmd & (1 << 11)) >> 11;

  // GP0(E1).12
  m_draw_mode.rect_textured_x_flip = (cmd & (1 << 12)) >> 12;
  // GP0(E1).13
  m_draw_mode.rect_textured_y_flip = (cmd & (1 << 13)) >> 13;
}

void Gpu::gp0_mask_bit(u32 cmd) {
  // GPUSTAT.11 = GP0(E6h).0
  m_gpustat.force_set_mask_bit = cmd & 1;
  // GPUSTAT.12 = GP0(E6h).1
  m_gpustat.preserve_masked_bits = cmd & 0b10 >> 1;
}

void Gpu::gp1(u32 cmd) {
  const auto opcode = (cmd >> 24) & 0xFF;
  const auto args = (cmd & 0xFFFFFF) >> 8;

  LOG_DEBUG("GP1 cmd: op: {:02X} args: {:06X}", opcode, args);

  switch (opcode) {
    case 0x00: gp1_soft_reset(); break;
    case 0x02: gp1_ack_gpu_interrupt(); break;
    case 0x03: gp1_disp_enable(cmd); break;
    case 0x04: gp1_dma_direction(cmd); break;
    case 0x05: m_display_area.word = cmd; break;
    case 0x06: m_hdisplay_range.word = cmd; break;
    case 0x07: m_vdisplay_range.word = cmd; break;
    case 0x08: gp1_disp_mode(cmd); break;
    default: LOG_WARN("Unhandled GP1 cmd: 0x{:08X}", cmd); break;
  }
}

void Gpu::gp1_soft_reset() {
  m_gpustat = GpuStatus();

  m_draw_mode = Gp0DrawMode();
  m_tex_window = Gp0TextureWindow();
  m_drawing_area_top_left = Gp0DrawingArea();
  m_drawing_area_bottom_right = Gp0DrawingArea();
  m_drawing_offset = Gp0DrawingOffset();

  m_display_area = Gp1DisplayArea();
  m_hdisplay_range = Gp1HDisplayRange();
  m_vdisplay_range = Gp1VDisplayRange();

  // TODO: flush cmd buffer
  // TODO: flush tex cache
}

void Gpu::gp1_disp_enable(u32 cmd) {
  m_gpustat.disp_disabled = cmd & 1;
}

void Gpu::gp1_dma_direction(u32 cmd) {
  m_gpustat.dma_direction_ = cmd & 0b11;
}

void Gpu::gp1_disp_mode(u32 cmd) {
  const u32 disp_mode_to_gpustat_mask = 0b111111u;
  const u32 gpustat_disp_mode_mask = 0b111111u << 17;

  // GPUSTAT.17-22 = GP1(E8).0-5
  m_gpustat.word &= ~gpustat_disp_mode_mask;
  m_gpustat.word |= (cmd & disp_mode_to_gpustat_mask) << 17;

  // GPUSTAT.16 = GP1(E8).6
  m_gpustat.horizontal_res_2 = (cmd & (1 << 6)) >> 6;
  // GPUSTAT.14 = GP1(E8).7
  m_gpustat.reverse_flag = (cmd & (1 << 7)) >> 7;
}

}  // namespace gpu
