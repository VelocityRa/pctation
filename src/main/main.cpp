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

    gui.init();

    std::string psxexe_path;
    std::string bootstrap_path;

    if (argc > 1)
      psxexe_path = argv[1];

    // TODO: do this properly
    if (argc > 2)
        bootstrap_path = argv[2];

    auto emulator = std::make_unique<emulator::Emulator>(BIOS_PATH, psxexe_path, bootstrap_path);

    bool running = true;
    while (running) {
      while (gui.poll_events()) {
        running = !gui.process_events(emulator->joypad());
      }

      emulator->advance_frame();

      gui.clear();
      emulator->draw();
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
