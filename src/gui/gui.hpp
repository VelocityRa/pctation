#pragma once

#include <util/types.hpp>

#include <SDL.h>
#include <SDL2/SDL_video.h>
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
  void init();
  bool poll_events();     // Returns true if there are any pending events
  bool process_events();  // returns true if apllication exit was requested
  void draw(const emulator::Emulator& emulator);
  void swap();
  void deinit();
  void clear();

 private:
  void draw_imgui(const emulator::Emulator& emulator);  // Draws all imgui GUI elements
  void draw_overlay_fps();
  void draw_dialog_tty(const char* tty_text);
  template <size_t RamSize>
  void draw_dialog_ram(const std::array<byte, RamSize>& data);
  void draw_gpu_registers(const gpu::Gpu& gpu);

 private:
  // SDL
  SDL_Window* m_window;
  SDL_GLContext m_gl_context;
  SDL_Event m_event;

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
