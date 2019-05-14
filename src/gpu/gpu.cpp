#include <gpu/gpu.hpp>

#include <util/bit_utils.hpp>
#include <util/log.hpp>

#include <gsl-lite.hpp>

#include <bitset>
#include <functional>
#include <unordered_map>

namespace gpu {

Gpu::Gpu() {
  m_gp0_cmd.reserve(MAX_GP0_CMD_LEN);
  m_vram = std::make_unique<std::array<u16, VRAM_WIDTH * VRAM_HEIGHT>>();
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

u16 Gpu::get_vram_pos(u16 x, u16 y) const {
  Ensures(x <= 1024);
  Ensures(y <= 512);

  const auto idx = x + y * VRAM_WIDTH;

  return get_vram_idx(idx);
}

u16 Gpu::get_vram_idx(u32 vram_idx) const {
  return vram()[vram_idx];
}

void Gpu::set_vram_pos(u16 x, u16 y, u16 val, bool wrap) {
  if (wrap) {
    x %= VRAM_WIDTH;
    y %= VRAM_HEIGHT;
  }

  Ensures(x <= 1024);
  Ensures(y <= 512);

  const auto idx = x + y * VRAM_WIDTH;

  set_vram_idx(idx, val);
}

void Gpu::set_vram_idx(u32 vram_idx, u16 val) {
  vram()[vram_idx] = val;
}

bool Gpu::step(u32 cycles_to_emulate) {
  m_vblank_cycles_left -= cycles_to_emulate;

  bool trigger_vblank = (m_vblank_cycles_left <= 0);

  if (trigger_vblank)
    m_vblank_cycles_left += CPU_CYCLES_PER_FRAME;

  return trigger_vblank;
}

u32 Gpu::setup_vram_transfer(u32 pos_word, u32 size_word) {
  m_vram_transfer_x = pos_word & 0x3FF;
  m_vram_transfer_y = (pos_word >> 16) & 0x1FF;

  m_vram_transfer_width = (((size_word & 0xFFFF) - 1) & 0x3FF) + 1;
  m_vram_transfer_height = ((((size_word >> 16) & 0xFFFF) - 1) & 0x1FF) + 1;

  m_vram_transfer_x_start = m_vram_transfer_x;

  const auto pixel_count = (m_vram_transfer_width * m_vram_transfer_height + 1) & ~1u;
  return pixel_count;
}

void Gpu::advance_vram_transfer_pos() {
  const auto rect_x = m_vram_transfer_x - m_vram_transfer_x_start;
  if (rect_x == m_vram_transfer_width - 1) {
    m_vram_transfer_x = m_vram_transfer_x_start;
    m_vram_transfer_y++;
  } else
    m_vram_transfer_x++;
}

DisplayResolution Gpu::get_resolution() const {
  DisplayResolution res;

  if (m_gpustat.horizontal_res_2 == 1)
    res.width = 368;
  else
    switch (m_gpustat.horizontal_res_1) {
      case 0: res.width = 256; break;
      case 1: res.width = 320; break;
      case 2: res.width = 512; break;
      case 3: res.width = 640; break;
    }

  if (m_gpustat.vertical_res || !m_gpustat.vertical_interlace)
    res.height = 240;
  else
    res.height = 480;
  return res;
}

void Gpu::gp0(u32 cmd) {
  if (m_gp0_cmd_type == Gp0CommandType::None) {
    m_gp0_cmd.clear();
    m_gp0_cmd.push_back(cmd);

    const u8 opcode = cmd >> 24;
    const u32 args = cmd & 0xFFFFFF;
    m_gp0_arg_index = 0;
    m_gp0_arg_count = 0;

    LOG_DEBUG("GP0 cmd: {:08X}", cmd);

    if (opcode == 0x00) {         // Nop
    } else if (opcode == 0x02) {  // Fill Rectangle in VRAM
      m_gp0_cmd_type = Gp0CommandType::FillRectangleInVram;
      m_gp0_arg_count = 2;
    } else if (opcode == 0x01) {                   // Clear cache
    } else if (0x20 <= opcode && opcode < 0x40) {  // Draw polygon
      m_gp0_cmd_type = Gp0CommandType::DrawPolygon;
      m_gp0_arg_count = renderer::rasterizer::DrawCommand{ opcode }.polygon.get_arg_count();
    } else if (0x40 <= opcode && opcode < 0x60) {  // Draw line
      m_gp0_cmd_type = Gp0CommandType::DrawLine;
      m_gp0_arg_count = renderer::rasterizer::DrawCommand{ opcode }.line.get_arg_count();
    } else if (0x60 <= opcode && opcode < 0x80) {  // Draw rectangle
      m_gp0_cmd_type = Gp0CommandType::DrawRectangle;
      m_gp0_arg_count = renderer::rasterizer::DrawCommand{ opcode }.rectangle.get_arg_count();
    } else if (opcode == 0x1F)
      gp0_gpu_irq(cmd);
    else if (opcode == 0xA0) {  // Copy rectangle (CPU -> VRAM)
      m_gp0_cmd_type = Gp0CommandType::CopyCpuToVram;
      m_gp0_arg_count = 2;
    } else if (opcode == 0xC0) {  // Copy rectangle (VRAM -> CPU)
      m_gp0_cmd_type = Gp0CommandType::CopyVramToCpu;
      m_gp0_arg_count = 2;
    } else if (opcode == 0xE1)
      gp0_draw_mode(cmd);
    else if (opcode == 0xE2)
      gp0_texture_window(cmd);
    else if (opcode == 0xE3)
      gp0_drawing_area_top_left(cmd);
    else if (opcode == 0xE4)
      gp0_drawing_area_bottom_right(cmd);
    else if (opcode == 0xE5)
      gp0_drawing_offset(cmd);
    else if (opcode == 0xE6)
      gp0_mask_bit(cmd);
    else {  // command is unimplemented
      // Error here because if this command accepts parameters, we will process them as commands and
      // cause undefined behavior
      LOG_ERROR("Unhandled GP0 cmd: 0x{:08X}", cmd);
    }
    return;
  }

  // If it reaches here we know 'cmd' is an argument to some preceding command, or CPU -> VRAM transfer
  // image data

  m_gp0_arg_index++;
  LOG_TRACE("  GP0 arg: {:08X}", cmd);

  const bool is_transfer_data = (m_gp0_cmd_type == Gp0CommandType::CopyCpuToVramTransferring);

  if (is_transfer_data) {
    do_cpu_to_vram_transfer(cmd);
    return;
  }

  // If it reaches here we know 'cmd' is an argument to some preceding command

  m_gp0_cmd.push_back(cmd);

  bool command_issued = (m_gp0_arg_index == m_gp0_arg_count);

  if (m_gp0_arg_count == MAX_GP0_CMD_LEN - 1)
    if (!command_issued)
      if (cmd == 0x55555555 || cmd == 0x50005000)
        command_issued = true;

  if (command_issued) {
    // Save temporary and reset it here instead of at the end, because following
    // handlers might change it themselves type and we wouldn't want to override that
    const auto cmd_type = m_gp0_cmd_type;
    m_gp0_cmd_type = Gp0CommandType::None;

    // We have all the arguments, we can run the command
    switch (cmd_type) {
      case Gp0CommandType::DrawPolygon: {
        const u8 opcode = m_gp0_cmd[0] >> 24;
        auto draw_cmd = renderer::rasterizer::DrawCommand{ opcode }.polygon;
        m_rasterizer.draw_polygon(draw_cmd);
        break;
      }
      case Gp0CommandType::DrawLine: {
        const u8 opcode = m_gp0_cmd[0] >> 24;
        auto draw_cmd = renderer::rasterizer::DrawCommand{ opcode }.line;
        // TODO:
        LOG_WARN("Unimplemented rendering of {} line (op: {:02X})",
                 draw_cmd.is_poly() ? "poly" : "single", opcode);
        break;
      }
      case Gp0CommandType::DrawRectangle: {
        const u8 opcode = m_gp0_cmd[0] >> 24;
        auto draw_cmd = renderer::rasterizer::DrawCommand{ opcode }.rectangle;
        m_rasterizer.draw_rectangle(draw_cmd);
        break;
      }
      case Gp0CommandType::FillRectangleInVram: gp0_fill_rect_in_vram(); break;
      case Gp0CommandType::CopyCpuToVram: gp0_copy_rect_cpu_to_vram(); break;
      case Gp0CommandType::CopyVramToCpu: gp0_copy_rect_vram_to_cpu(); break;
      case Gp0CommandType::Invalid: break;
    }
  }
}

void Gpu::gp0_mono_polyline_opaque(u32 cmd) {
  LOG_TODO();
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

void Gpu::gp0_fill_rect_in_vram() {
  // TODO: handle in renderer
  const auto color = renderer::rasterizer::Color::from_gp0(m_gp0_cmd[0]);
  const auto c16 = RGB16::from_RGB(color.r, color.g, color.b);

  const auto pos_start = renderer::rasterizer::Position::from_gp0_fill(m_gp0_cmd[1]);
  const auto size = renderer::rasterizer::Size::from_gp0_fill(m_gp0_cmd[2]);
  const renderer::rasterizer::Position pos_end = { pos_start.x + size.width, pos_start.y + size.height };

  for (auto i_x = pos_start.x; i_x < pos_end.x; ++i_x)
    for (auto i_y = pos_start.y; i_y < pos_end.y; ++i_y)
      set_vram_pos(i_x, i_y, c16.word, true);
}

void Gpu::gp0_copy_rect_cpu_to_vram() {
  const auto pos_word = m_gp0_cmd[1];
  const auto size_word = m_gp0_cmd[2];

  const auto pixel_count = setup_vram_transfer(pos_word, size_word);

  // Reset arg index, we are now counting transfer words, not the command's 2 arguments
  m_gp0_arg_index = 0;
  // Divide by two since packets are 32-bit and we have a 16-bit size
  m_gp0_arg_count = pixel_count / 2;
  // Next GP0 packets will contain image data
  m_gp0_cmd_type = Gp0CommandType::CopyCpuToVramTransferring;

  LOG_DEBUG("Copying rect (x:{} y:{} w:{} h:{} count:{} hw) from CPU to VRAM", m_vram_transfer_x,
            m_vram_transfer_y, m_vram_transfer_width, m_vram_transfer_height, pixel_count);
}

void Gpu::gp0_copy_rect_vram_to_cpu() {
  const auto pos_word = m_gp0_cmd[1];
  const auto size_word = m_gp0_cmd[2];

  const auto pixel_count = setup_vram_transfer(pos_word, size_word);

  LOG_DEBUG("Copying rect (x:{} y:{} w:{} h:{} count:{} hw) from VRAM to CPU", m_vram_transfer_x,
            m_vram_transfer_y, m_vram_transfer_width, m_vram_transfer_height, pixel_count);
}

void Gpu::do_cpu_to_vram_transfer(u32 cmd) {
  for (auto i = 0; i < 2; ++i) {
    u16 src_word;
    if (i == 0)
      src_word = (u16)cmd;
    else
      src_word = (u16)(cmd >> 16);

    //        LOG_TRACE("X: {:>4X} Y: {:>4X} SRC: 0x{:04X}", m_vram_transfer_x, m_vram_transfer_y,
    //        src_word);

    set_vram_pos(m_vram_transfer_x, m_vram_transfer_y, src_word, true);
    advance_vram_transfer_pos();
  }
  if (m_gp0_arg_index == m_gp0_arg_count) {
    // Load done, start processing new commands
    m_gp0_cmd_type = Gp0CommandType::None;
  }
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
  m_drawing_offset.x = bit_utils::sign_extend<11, u32>(m_drawing_offset.x);
  m_drawing_offset.y = bit_utils::sign_extend<11, u32>(m_drawing_offset.y);
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
  m_gp0_cmd_type = Gp0CommandType::None;
  m_gp0_arg_count = 0;
  m_gp0_arg_index = 0;
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
