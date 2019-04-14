#include <emulator/emulator.hpp>
#include <gui/gui.hpp>
#include <util/log.hpp>

#include <cassert>
#include <exception>
#include <memory>

constexpr auto BIOS_PATH = "../../data/bios/SCPH1001.BIN";

// Entry point
s32 main(s32 argc, char** argv) {
  gui::Gui gui;

  try {
    logging::init();

    gui.init();

    std::string psxexe_path;

    if (argc > 1)
      psxexe_path = argv[1];

    auto emulator = std::make_unique<emulator::Emulator>(BIOS_PATH, psxexe_path);

    bool running = true;
    while (running) {
      while (gui.poll_events()) {
        running = !gui.process_events();
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
