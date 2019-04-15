#include <gpu/gpu.hpp>
#include <util/log.hpp>

#include <gsl-lite.hpp>

#include <bitset>
#include <functional>
#include <unordered_map>

namespace gpu {

Gpu::Gpu() {
  m_gp0_cmd.reserve(12);  // Max command size
}

u32 Gpu::read_reg(u32 addr) {
  switch (addr) {
    case 0: LOG_WARN("Unhandled 32-bit read of GPUREAD"); return 0;
    case 4: return gpustat().word;
  }
  // Unreachable because the GPU Range only includes the above 2 registers
  return 0;
}

void Gpu::write_reg(u32 addr, u32 val) {
  switch (addr) {
    case 0: gp0(val); break;
    case 4: gp1(val); break;
  }
}

void Gpu::draw() {
  m_renderer.draw();
}

void Gpu::gp0(u32 cmd) {
  bool is_new_command = m_gp0_words_left == 0;

  if (is_new_command) {
    const auto opcode = (cmd >> 24) & 0xFF;
    const auto args = cmd & 0xFFFFFF;

    LOG_DEBUG("GP0 cmd: {:08X}", cmd);

    // Pair of command length (in words) and handler
    using Gp0OpcodeHandlerEntry = std::pair<u8, GpuGp0CmdHandler>;

    static const std::unordered_map<u8, Gp0OpcodeHandlerEntry> gp0_handlers = {
      { 0x00, { 1, &Gpu::gp0_nop } },
      { 0x01, { 1, &Gpu::gp0_clear_cache } },
      { 0x28, { 5, &Gpu::gp0_quad_mono_opaque } },
      { 0x2C, { 9, &Gpu::gp0_quad_texture_blend_opaque } },
      { 0x30, { 6, &Gpu::gp0_triangle_shaded_opaque } },
      { 0x38, { 8, &Gpu::gp0_quad_shaded_opaque } },
      { 0x1F, { 1, &Gpu::gp0_gpu_irq } },
      { 0x68, { 2, &Gpu::gp0_mono_rect_1x1_opaque } },
      { 0xA0, { 3, &Gpu::gp0_copy_rect_cpu_to_vram } },
      { 0xC0, { 3, &Gpu::gp0_copy_rect_vram_to_cpu } },
      { 0xE1, { 1, &Gpu::gp0_draw_mode } },
      { 0xE2, { 1, &Gpu::gp0_texture_window } },
      { 0xE3, { 1, &Gpu::gp0_drawing_area_top_left } },
      { 0xE4, { 1, &Gpu::gp0_drawing_area_bottom_right } },
      { 0xE5, { 1, &Gpu::gp0_drawing_offset } },
      { 0xE6, { 1, &Gpu::gp0_mask_bit } },
    };

    const auto handler_entry = gp0_handlers.find(opcode);
    if (handler_entry != std::end(gp0_handlers)) {  // we have a handler for this command
      const auto cmd_length = handler_entry->second.first;
      const auto cmd_handler = handler_entry->second.second;

      m_gp0_words_left = cmd_length;
      m_gp0_handler = cmd_handler;

      m_gp0_cmd.clear();
    } else {  // command is unimplemented
      // Error here because if this command accepts parameters, we will process them as commands and
      // cause undefined behavior
      LOG_ERROR("Unhandled GP0 cmd: 0x{:08X}", cmd);
    }
  } else {  // argument to some preceding command
    LOG_TRACE("  arg: {:08X}", cmd);
  }

  m_gp0_words_left--;

  bool transfer_finished = m_gp0_words_left == 0;

  switch (m_gp0_mode) {
    case Gp0Mode::Command:
      m_gp0_cmd.push_back(cmd);

      if (transfer_finished) {
        // We have all the arguments, we can run the command
        std::invoke(m_gp0_handler, this, cmd);
      }
      break;
    case Gp0Mode::ImageLoad:
      // TODO: copy to vram

      if (transfer_finished) {
        // Load done, switch back to command mode
        m_gp0_mode = Gp0Mode::Command;
      }
      break;
    default: LOG_ERROR("Invalid Gp0Mode"); assert(0);
  }
}

void Gpu::gp0_quad_mono_opaque(u32 cmd) {
  const auto color = m_gp0_cmd[0];
  m_renderer.draw_quad_mono(
      renderer::Position::from_gp0(m_gp0_cmd[1], m_gp0_cmd[2], m_gp0_cmd[3], m_gp0_cmd[4]),
      renderer::Color::from_gp0(color));
}

void Gpu::gp0_quad_texture_blend_opaque(u32 cmd) {
  // TODO: We don't support textures yet, so use a solid blue color for now
  m_renderer.draw_quad_mono(
      renderer::Position::from_gp0(m_gp0_cmd[1], m_gp0_cmd[3], m_gp0_cmd[5], m_gp0_cmd[7]),
      renderer::Color{ 0, 0, 255 });
}

void Gpu::gp0_triangle_shaded_opaque(u32 cmd) {
  m_renderer.draw_triangle_shaded(renderer::Position::from_gp0(m_gp0_cmd[1], m_gp0_cmd[3], m_gp0_cmd[5]),
                                  renderer::Color::from_gp0(m_gp0_cmd[0], m_gp0_cmd[2], m_gp0_cmd[4]));
}

void Gpu::gp0_quad_shaded_opaque(u32 cmd) {
  m_renderer.draw_quad_shaded(
      renderer::Position::from_gp0(m_gp0_cmd[1], m_gp0_cmd[3], m_gp0_cmd[5], m_gp0_cmd[7]),
      renderer::Color::from_gp0(m_gp0_cmd[0], m_gp0_cmd[2], m_gp0_cmd[4], m_gp0_cmd[6]));
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

void Gpu::gp0_gpu_irq(u32 cmd) {
  m_gpustat.interrupt = true;
}

void Gpu::gp0_mono_rect_1x1_opaque(u32 cmd) {
  m_renderer.draw_pixel(renderer::Position::from_gp0(m_gp0_cmd[1]),
                        renderer::Color::from_gp0(m_gp0_cmd[0]));
}

void Gpu::gp0_copy_rect_cpu_to_vram(u32 cmd) {
  const auto pos_word = m_gp0_cmd[1];
  const auto size_word = m_gp0_cmd[2];

  const auto width = size_word & 0xFFFF;
  const auto height = size_word >> 16;

  // Size of image in 16-bit pixels, rounded up to nearest 32-bit value
  const auto size = (width * height + 1) & ~1;

  m_gp0_words_left = size / 2;  // Divide by two since packets are 32-bit and we have a 16-bit size

  // Next GP0 packets will contain image data
  m_gp0_mode = Gp0Mode::ImageLoad;

  // TODO: impl VRAM
  // TODO: copy data to VRAM
}

void Gpu::gp0_copy_rect_vram_to_cpu(u32 cmd) {
  const auto pos_word = m_gp0_cmd[1];
  const auto size_word = m_gp0_cmd[2];

  const auto width = size_word & 0xFFFF;
  const auto height = size_word >> 16;

  // Size of image in 16-bit pixels, rounded up to nearest 32-bit value
  const auto size = (width * height + 1) & ~1;

  LOG_WARN(__FUNCTION__ ", width: {} height: {} size {} TODO", width, height, size);
}

void Gpu::gp0_texture_window(u32 cmd) {
  m_tex_window.word = cmd;
}

void Gpu::gp0_drawing_area_top_left(u32 cmd) {
  m_drawing_area_top_left.word = cmd;
}

void Gpu::gp0_drawing_area_bottom_right(u32 cmd) {
  m_drawing_area_bottom_right.word = cmd;
}

void Gpu::gp0_drawing_offset(u32 cmd) {
  m_drawing_offset.word = cmd;

  // HACK: trigger frame render
  m_frame = true;
}

void Gpu::gp1(u32 cmd) {
  const auto opcode = (cmd >> 24) & 0xFF;
  const auto args = cmd & 0xFFFFFF;

  LOG_DEBUG("GP1 cmd: op: {:02X} args: {:06X}", opcode, args);

  switch (opcode) {
    case 0x00: gp1_soft_reset(); break;
    case 0x01: gp1_cmd_buf_reset(); break;
    case 0x02: gp1_ack_gpu_interrupt(); break;
    case 0x03: gp1_disp_enable(cmd); break;
    case 0x04: gp1_dma_direction(cmd); break;
    case 0x05:
      Ensures((cmd & 0xFF) == 0);
      m_display_area.word = cmd;
      break;
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

  gp1_cmd_buf_reset();

  // TODO: flush tex cache
}

void Gpu::gp1_cmd_buf_reset() {
  // Clear GP0 state machine state
  // TODO: Implement proper FIFO
  m_gp0_cmd.clear();
  m_gp0_mode = Gp0Mode::Command;
  m_gp0_words_left = 0;
}

void Gpu::gp1_ack_gpu_interrupt() {
  m_gpustat.interrupt = false;
}

void Gpu::gp1_disp_enable(u32 cmd) {
  m_gpustat.disp_disabled = cmd & 1;
}

void Gpu::gp1_dma_direction(u32 cmd) {
  m_gpustat.dma_direction_ = cmd & 0b11;
  u8 dma_req{};

  switch (m_gpustat.dma_direction()) {
    case Off: dma_req = 0; break;
    case Fifo: dma_req = 1; break;
    case CpuToGp0: dma_req = m_gpustat.ready_to_recv_dma_block; break;
    case VRamToCpu: dma_req = m_gpustat.ready_to_send_vram_to_cpu; break;
  }

  std::bitset<32> gpustat_bs(m_gpustat.word);
  gpustat_bs.set(26, dma_req);
  m_gpustat.word = gpustat_bs.to_ulong();
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
