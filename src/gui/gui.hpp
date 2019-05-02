#pragma once

#include <util/types.hpp>

#include <SDL.h>
#include <SDL2/SDL_video.h>
#include <imgui.h>
#include <imgui_memory_editor/imgui_memory_editor.h>

#include <chrono>
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
  void draw_dialog_log(const char* title,
                       bool& should_draw,
                       bool& should_autoscroll,
                       const char* text_contents);
  template <size_t RamSize>
  void draw_dialog_ram(const std::array<byte, RamSize>& data);
  void draw_gpu_registers(const gpu::Gpu& gpu);
  void update_window_title();
  void update_fps_counter();

 private:
  // SDL
  SDL_Window* m_window;
  SDL_GLContext m_gl_context;
  SDL_Event m_event;

 private:
  // FPS counter fields
  u32 m_fps_counter_frames{};
  std::chrono::time_point<std::chrono::steady_clock> m_fps_counter_start{};
  f32 m_fps{};

  // TTY dialog fields
  bool m_draw_tty{ true };
  bool m_tty_autoscroll{ true };

  // Bios Calls dialog fields
  bool m_draw_bios_calls{ true };
  bool m_bios_calls_autoscroll{ true };

  // RAM Memory dialog fields
  bool m_draw_ram{ true };
  MemoryEditor m_ram_memeditor;

  // GPU Registers dialog fields
  bool m_draw_gpu_registers{ true };
};

}  // namespace gui
