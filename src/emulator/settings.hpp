#pragma once

#include <util/types.hpp>

namespace emulator {

enum class View : s32 {
  Vram,     // Show the contents of the entire VRAM
  Display,  // Show only the display output that would be shown on a real console

  Maximum,
};

enum class ScreenScale : s32 { x1, x2, x3, x4 };

struct Settings {
  View screen_view{ View::Vram };
  bool window_size_changed{};

  ScreenScale screen_scale{ ScreenScale::x1 };

  // Resolutions below are technically redundant, used to pass new res from Emulator to Gui
  // Changed when screen_view changes
  u32 res_width;
  u32 res_height;

  bool show_gui{ true };

  bool limit_framerate{ true };
  bool limit_framerate_changed{};
};

}  // namespace emulator
