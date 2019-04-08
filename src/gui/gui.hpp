#pragma once

#include <util/types.hpp>

#include <imgui.h>
#include <imgui_memory_editor/imgui_memory_editor.h>

#include <string>

namespace emulator {
class Emulator;
}

namespace gpu {
class Gpu;
}

namespace gui {

class Gui {
 public:
  void draw(const emulator::Emulator& emulator);

 private:
  void draw_overlay_fps();
  void draw_dialog_tty(const char* tty_text);
  template <size_t RamSize>
  void draw_dialog_ram(const std::array<byte, RamSize>& data);
  void draw_gpu_registers(const gpu::Gpu& gpu);

 private:
  // Counts times draw() has been called
  // For updating some UI elements more rarely
  u32 m_draw_counter{};
  // Width of characters in current font (assumes it's monospaced)

  // FPS Overlay fields
  std::string m_fps_overlay_str;

  // TTY dialog fields
  bool m_draw_tty{ true };
  bool m_tty_autoscroll{ true };

  // RAM Memory dialog fields
  bool m_draw_ram{ true };
  MemoryEditor m_ram_memeditor;

  // GPU Registers dialog fields
  bool m_draw_gpu_registers{ true };
};

}  // namespace gui
