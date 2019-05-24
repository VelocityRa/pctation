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
struct Settings;
}  // namespace emulator

namespace cpu {
class Cpu;
}

namespace gpu {
class Gpu;
}

namespace io {
class Joypad;
class Timers;
}  // namespace io

namespace gui {

enum class GuiEvent {
  None,
  Exit,
  GameSelected,
};

class Gui {
 public:
  void init();
  void set_joypad(io::Joypad* joypad);
  void set_settings(emulator::Settings* joypad);
  void set_game_title(const std::string& game_title);
  void apply_settings() const;
  bool poll_events();  // Returns true if there are any pending events
  GuiEvent process_events();
  GuiEvent process_events_file_select() const;
  void draw(const emulator::Emulator& emulator);
  void draw_file_select(gui::Gui& gui, std::string& exe_path, std::string& bin_path);
  void swap();
  void deinit();
  void clear() const;

 private:
  void imgui_start_frame() const;
  void imgui_end_frame() const;
  void imgui_draw(const emulator::Emulator& emulator);  // Draws all imgui GUI elements

  void update_window_title() const;
  void update_fps_counter();
  void process_exit_events(GuiEvent& ret_event) const;

  // GUI Components

  bool draw_window_exe_select(std::string& psxexe_path) const;    // Returns true if a file was selected
  bool draw_window_cdrom_select(std::string& psxexe_path) const;  // Returns true if a file was selected

  void draw_window_log(const char* title,
                       bool& should_draw,
                       bool& should_autoscroll,
                       const char* text_contents) const;
  template <size_t RamSize>
  void draw_window_ram(const std::array<byte, RamSize>& data);
  void draw_window_gpu_registers(const gpu::Gpu& gpu);
  void draw_window_cpu_registers(const cpu::Cpu& cpu);
  void draw_window_gp0_commands(const gpu::Gpu& gpu);
  void draw_window_timers(const io::Timers& timers);

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

  // TTY window fields
  bool m_draw_tty{ true };
  bool m_tty_autoscroll{ true };

  // Bios Calls window fields
  bool m_draw_bios_calls{ true };
  bool m_bios_calls_autoscroll{ true };

  // RAM Memory window fields
  bool m_draw_ram{ true };
  MemoryEditor m_ram_memeditor;

  // GPU Registers window fields
  bool m_draw_gpu_registers{ true };

  // CPU execution window fields
  bool m_draw_cpu_registers{ true };

  // GP0 Commands window fields
  bool m_draw_gp0_commands{ true };
  bool m_draw_gp0_overlay_rising{ true };
  u8 m_draw_gp0_overlay_alpha{};

  // Timers window fields
  bool m_draw_timers{ true };

  std::string m_game_title;

  io::Joypad* m_joypad;
  emulator::Settings* m_settings;
};

}  // namespace gui
