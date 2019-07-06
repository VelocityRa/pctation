#include <emulator/emulator.hpp>

#include <gui/gui.hpp>
#include <util/log.hpp>

#include <cassert>
#include <exception>
#include <memory>
#include <tuple>

constexpr auto NOCASH_BIOS_2_0_PATH = "data/bios/no$psx_bios/NO$PSX_BIOS_2.0_2x.ROM";
constexpr auto NOCASH_BIOS_1_2_PATH = "data/bios/no$psx_bios/NO$PSX_BIOS_1.2_2x.ROM";
constexpr auto BIOS_PATH = "data/bios/SCPH1001.BIN";

// Entry point
s32 main(s32 argc, char** argv) {
  gui::Gui gui;

  try {
    logging::init();

    std::string bootstrap_path;
    std::string exe_path;
    std::string cdrom_path;  // Either a cue sheet or a raw CD-ROM binary file

    // TODO: do this properly

    if (argc > 1)
      cdrom_path = argv[1];

//    if (argc > 2)
//      exe_path = argv[2];
//
//    if (argc > 3)
//      bootstrap_path = argv[3];

    gui.init();

    // If no executable was specified in cmd args, show Executable Select screen
    if (exe_path.empty() && cdrom_path.empty())
      gui.draw_file_select(gui, exe_path, cdrom_path);

    // Init emulator
    auto emulator =
        std::make_unique<emulator::Emulator>(BIOS_PATH, exe_path, bootstrap_path, cdrom_path);

    // Update window with exe/game title
    if (!cdrom_path.empty())
      gui.set_game_title(fs::path(cdrom_path).stem().string());
    else if (!exe_path.empty())
      gui.set_game_title(fs::path(exe_path).stem().string());

    // Link GUI with Emulator
    gui.set_joypad(&emulator->joypad());
    gui.set_settings(&emulator->settings());

    // Main loop
    auto event = gui::GuiEvent::None;

    while (true) {
      while (gui.poll_events()) {
        event = gui.process_events();

        if (event == gui::GuiEvent::Exit)
          return 0;
      }
      emulator->update_settings();
      gui.apply_settings();

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
