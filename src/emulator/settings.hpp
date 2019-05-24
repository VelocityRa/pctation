#pragma once

#include <util/types.hpp>

namespace emulator {

enum class View : s32 {
  Vram,     // Show the contents of the entire VRAM
  Display,  // Show only the display output that would be shown on a real console

  Maximum,
};

enum class ScreenScale : s32 { x1, x1_5, x2, x3, x4 };

struct Settings {
  View screen_view{ View::Vram };
  bool window_size_changed{ true };

  ScreenScale screen_scale{ ScreenScale::x1_5 };

  // Resolutions below are technically redundant, used to pass new res from Emulator to Gui
  // Changed when screen_view changes
  u32 res_width;
  u32 res_height;

  bool show_gui{ true };

  bool limit_framerate{ false };
  bool limit_framerate_changed{ true };

  // Logging
  bool log_trace_cpu{};

  f32 get_screen_scale() const {
    switch (screen_scale) {
      case ScreenScale::x1: return 1;
      case ScreenScale::x1_5: return 1.5;
      case ScreenScale::x2: return 2;
      case ScreenScale::x3: return 3;
      case ScreenScale::x4: return 4;
      default: return 1;
    }
  }
};

}  // namespace emulator
