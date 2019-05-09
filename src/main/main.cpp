#include <emulator/emulator.hpp>
#include <gui/gui.hpp>
#include <util/log.hpp>

#include <cassert>
#include <exception>
#include <memory>

constexpr auto NOCASH_BIOS_2_0_PATH = "data/bios/no$psx_bios/NO$PSX_BIOS_2.0_2x.ROM";
constexpr auto NOCASH_BIOS_1_2_PATH = "data/bios/no$psx_bios/NO$PSX_BIOS_1.2_2x.ROM";
constexpr auto BIOS_PATH = "data/bios/SCPH1001.BIN";

// Entry point
s32 main(s32 argc, char** argv) {
  gui::Gui gui;

  try {
    logging::init();

    std::string exe_path;
    std::string bootstrap_path;

    if (argc > 1)
      psxexe_path = argv[1];

    // TODO: do this properly
    //    if (argc > 2)
    //      bootstrap_path = argv[2];

    gui.init();

    // Init emulator
    auto emulator = std::make_unique<emulator::Emulator>(BIOS_PATH, exe_path, bootstrap_path);

    // Link GUI with Joypad
    gui.set_joypad(&emulator->joypad());

    // Main loop
    gui::GuiEvent event = gui::GuiEvent::None;

    while (event != gui::GuiEvent::Exit) {
      while (gui.poll_events()) {
        event = gui.process_events();

        if (event == gui::GuiEvent::ToggleView) {
          const auto size = emulator->toggle_view();
          gui.set_window_size(size);
        }
      }

      emulator->advance_frame();

      gui.clear();
      emulator->render();
      gui.draw(*emulator);

      gui.swap();
    }
  } catch (const std::exception& e) {
    LOG_CRITICAL("Exception: {}", e.what());
    gui.deinit();
    assert(0);
  }

  return 0;
}
