#pragma once

#include <util/types.hpp>

#include <SDL.h>
#include <SDL2/SDL_video.h>
#include <imgui.h>
#include <imgui_memory_editor/imgui_memory_editor.h>

#include <chrono>
#include <string>

namespace io {
class Joypad;
}

namespace emulator {
class Emulator;
struct WindowSize;
}  // namespace emulator

namespace gpu {
class Gpu;
}

namespace gui {

enum class GuiEvent {
  None,
  Exit,
  ToggleView,
};

class Gui {
 public:
  void init();
  void set_joypad(io::Joypad* joypad);
  bool poll_events();  // Returns true if there are any pending events
  GuiEvent process_events() const;
  GuiEvent process_events_exe_select();
  void draw(const emulator::Emulator& emulator);
  void process_exit_events(GuiEvent& ret_event) const;
  void swap();
  void set_window_size(emulator::WindowSize size);
  void deinit();
  void clear();

 private:
  void imgui_start_frame() const;
  void imgui_end_frame() const;
  void imgui_draw(const emulator::Emulator& emulator);  // Draws all imgui GUI elements

  void draw_dialog_log(const char* title,
                       bool& should_draw,
                       bool& should_autoscroll,
                       const char* text_contents);
  template <size_t RamSize>
  void draw_dialog_ram(const std::array<byte, RamSize>& data);
  void draw_gpu_registers(const gpu::Gpu& gpu);
  void update_window_title() const;
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

  io::Joypad* m_joypad;
};

}  // namespace gui
