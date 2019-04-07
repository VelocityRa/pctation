#pragma once

#include <imgui.h>
#include <imgui_memory_editor/imgui_memory_editor.h>

#include <string>

namespace emulator {
class Emulator;
}

namespace gui {

class Gui {
 public:
  void show(const emulator::Emulator& emulator);

 private:
  void show_overlay_fps();
  void show_dialog_tty(const char* tty_text);
  template <size_t RamSize>
  void show_dialog_ram(const std::array<byte, RamSize>& data);

 private:
  // Counts times show() has been called
  // For updating some UI elements more rarely
  u32 m_show_counter{};

 // FPS Overlay
 std::string m_fps_overlay_str;

  // TTY dialog
  bool m_show_tty{ true };
  bool m_tty_autoscroll{ true };

  // RAM Memory dialog
  bool m_show_ram{ true };
  MemoryEditor m_ram_memeditor;
};

}  // namespace gui
