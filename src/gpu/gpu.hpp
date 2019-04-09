#pragma once

#include <util/types.hpp>

#include <gsl-lite.hpp>

#include <vector>

namespace gpu {

enum DmaDirection {
  Off = 0,
  Fifo = 1,
  CpuToGp0 = 2,
  VRamToCpu = 3,
};

enum class Gp0Mode {
  Command,    // We're processing words that are part of a command
  ImageLoad,  // We're processing words that are image data transferred by a Copy command
};

// GP0(E2h) - Texture Window setting
union Gp0TextureWindow {
  u32 word{};

  struct {
    u32 tex_window_mask_x : 5;  //  0-4    Texture window Mask X   (in 8 pixel steps)
    u32 tex_window_mask_y : 5;  //  5-9    Texture window Mask Y   (in 8 pixel steps)
    u32 tex_window_off_x : 5;   //  10-14  Texture window Offset X (in 8 pixel steps)
    u32 tex_window_off_y : 5;   //  15-19  Texture window Offset Y (in 8 pixel steps)
  };
};

// GP0(E3h) - Set Drawing Area top left (X1,Y1)
// GP0(E4h) - Set Drawing Area bottom right (X2,Y2)
union Gp0DrawingArea {
  u32 word{};

  struct {
    u32 x_coord : 10;  // 0-9    X-coordinate (0..1023)
    u32 y_coord : 10;  // 10-18  Y-coordinate (0..511)   ;\on Old 160pin GPU (max 1MB VRAM)
    u32 _19 : 1;       // 19-23  Not used (zero)         ; TODO: Used for y on new GPU
  };
};

// TODO: This has s11's, maybe store as separate field and not in bitfield
// GP0(E5h) - Set Drawing Offset (X,Y)
union Gp0DrawingOffset {
  u32 word{};

  struct {
    u32 x_off : 10;  // 0-10   X-offset (-1024..+1023) (usually within X1,X2 of Drawing Area)
    u32 y_off : 10;  // 11-21  Y-offset (-1024..+1023) (usually within Y1,Y2 of Drawing Area)
  };
};

struct Gp0DrawMode {
  // The rest of the fields are set to GPUSTAT
  bool rect_textured_x_flip;
  bool rect_textured_y_flip;
};

// GP1(05h) - Start of Display area (in VRAM)
union Gp1DisplayArea {
  u32 word{};

  struct {
    u32 x_coord : 10;  // X (0-1023)    (halfword address in VRAM)  (relative to begin of VRAM)
    u32 y_coord : 9;   // Y (0-511)     (scanline number in VRAM)   (relative to begin of VRAM)
  };
};

// GP1(06h) - Horizontal Display range (on Screen)
union Gp1HDisplayRange {
  u32 word{};

  struct {
    u32 x1 : 12;  // X1 (260h+0)       ;12bit       ;\counted in 53.222400MHz units
    u32 x2 : 12;  // X2 (260h+320*8)   ;12bit       ;/relative to HSYNC
  };
};

// GP1(07h) - Vertical Display range (on Screen)
union Gp1VDisplayRange {
  u32 word{};

  struct {
    u32 y1 : 10;  // Y1 (NTSC=88h-(224/2), (PAL=A3h-(264/2))  ;\scanline numbers on screen
    u32 y2 : 10;  // Y2 (NTSC=88h+(224/2), (PAL=A3h+(264/2))  ;/relative to VSYNC
  };
};

// GPUSTAT register
union GpuStatus {
  u32 word{ 0x14802000 };

  struct {
    u32 tex_page_x_base : 4;       //  0-3   Texture page X Base
    u32 tex_page_y_base : 1;       //  4     Texture page Y Base
    u32 semi_transparency : 2;     //  5-6   Semi Transparency
    u32 tex_page_colors : 2;       //  7-8   Texture page colors
    u32 dither_en : 1;             //  9     Dither 24bit to 15bit
    u32 drawing_to_disp_en : 1;    //  10    Drawing to display area
    u32 force_set_mask_bit : 1;    //  11    Set Mask-bit when drawing pixels
    u32 preserve_masked_bits : 1;  //  12    Draw Pixels
    u32 interlace_field : 1;       //  13    Interlace Field
    u32 reverse_flag : 1;          //  14    "Reverseflag"
    u32 tex_disable : 1;           //  15    Texture Disable
    u32 horizontal_res_2 : 1;      //  16    Horizontal Resolution 2
    u32 horizontal_res_1 : 2;      //  17-18 Horizontal Resolution 1
    u32 vertical_res : 1;          //  19    Vertical Resolution
    u32 video_mode : 1;            //  20    Video Mode (0=NTSC/60Hz, 1=PAL/50Hz)
    u32 disk_color_depth : 1;      //  21    Display Area Color Depth (0=15bit, 1=24bit)
    u32 vertical_interlace : 1;    //  22    Vertical Interlace (0=Off, 1=On)
    u32 disp_disabled : 1;         //  23    Display Status (0=Enabled, 1=Disabled)
    u32 interrupt : 1;             //  24    Interrupt Request (IRQ1) (0=Off, 1=IRQ)
    u32 dma_data_req : 1;       //  25    DMA / Data Request, meaning depends on GP1(04h) DMA Direction
    u32 ready_to_recv_cmd : 1;  //  26    Ready to receive Cmd Word
    u32 ready_to_send_vram_to_cpu : 1;  // 27    Ready to send VRAM to CPU
    u32 ready_to_recv_dma_block : 1;    // 28    Ready to receive DMA Block
    u32 dma_direction_ : 2;             // 29-30 DMA Direction (0=Off, 1=?, 2=CPUtoGP0, 3=GPUREADtoCPU)
    u32 interlace_drawing_mode : 1;     // 31    Drawing even/odd lines in interlace mode (0=Even, 1=Odd)
  };

  // methods
  DmaDirection dma_direction() const { return static_cast<DmaDirection>(dma_direction_); }
};

class Gpu {
 public:
  Gpu();

  GpuStatus m_gpustat{};

  Gp0TextureWindow m_tex_window{};
  Gp0DrawingArea m_drawing_area_top_left{};
  Gp0DrawingArea m_drawing_area_bottom_right{};
  Gp0DrawingOffset m_drawing_offset;
  Gp0DrawMode m_draw_mode;

  Gp1DisplayArea m_display_area;
  Gp1HDisplayRange m_hdisplay_range;
  Gp1VDisplayRange m_vdisplay_range;

  GpuStatus gpustat() const {
    auto gpustat = static_cast<u32>(m_gpustat.word);

    // Hardcode the following for now
    gpustat |= 1 << 26;  // Ready to receive command: true
    gpustat |= 1 << 27;  // Ready to send VRAM to CPU: true
    gpustat |= 1 << 28;  // Ready to receive DMA block: true
    // TODO: Do I need to set .19 to 0 as well?

    // Not sure what this is
    Ensures(m_gpustat.reverse_flag == false);

    return GpuStatus{ gpustat };
  }

  u32 read32(u32 addr);
  void write32(u32 addr, u32 val);

  void gp0(u32 cmd);

 private:
  void gp0_nop(u32 cmd){};
  void gp0_clear_cache(u32 cmd){};
  void gp0_quad_mono_opaque(u32 cmd);
  void gp0_quad_texture_blend_opaque(u32 cmd);
  void gp0_triangle_shaded_opaque(u32 cmd);
  void gp0_quad_shaded_opaque(u32 cmd);
  void gp0_draw_mode(u32 cmd);
  void gp0_mask_bit(u32 cmd);
  void gp0_gpu_irq(u32 cmd);  // rarely used
  void gp0_copy_rect_cpu_to_vram(u32 cmd);
  void gp0_copy_rect_vram_to_cpu(u32 cmd);

  void gp1(u32 cmd);
  void gp1_soft_reset();
  void gp1_cmd_buf_reset();
  void gp1_ack_gpu_interrupt();
  void gp1_disp_enable(u32 cmd);
  void gp1_dma_direction(u32 cmd);
  void gp1_disp_mode(u32 cmd);
  void gp0_texture_window(u32 cmd);
  void gp0_drawing_area_top_left(u32 cmd);
  void gp0_drawing_area_bottom_right(u32 cmd);
  void gp0_drawing_offset(u32 cmd);

 private:
  // GP0 command handling
  Gp0Mode m_gp0_mode = Gp0Mode::Command;
  std::vector<u32> m_gp0_cmd;  // 1 to 12 words comprising a GP0 command
  u32 m_gp0_words_left = 0;
  using GpuGp0CmdHandler = void (Gpu::*)(u32 cmd);
  GpuGp0CmdHandler m_gp0_handler{};
};

}  // namespace gpu
