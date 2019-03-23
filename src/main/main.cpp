#include <emulator.hpp>
#include <fs.hpp>
#include <log.hpp>

#include <cassert>
#include <exception>

int main() {
  try {
    emulator::Emulator emulator("../../data/bios/SCPH101.BIN");
    emulator.run();

  } catch (const std::exception& e) {
    LOG::critical("{}", e.what());
    assert(0);
  }

  return 0;
}
