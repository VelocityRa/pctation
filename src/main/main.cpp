#include <emulator/emulator.hpp>
#include <util/fs.hpp>
#include <util/log.hpp>

#include <cassert>
#include <exception>

int main() {
  try {
    spdlog::set_pattern("%^[--%L--]%$ %16s:%-3# %v");
    emulator::Emulator emulator("../../data/bios/SCPH101.BIN");
    emulator.run();

  } catch (const std::exception& e) {
    LOG_CRITICAL("{}", e.what());
    assert(0);
  }

  return 0;
}
