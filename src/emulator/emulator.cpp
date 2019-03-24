#include <emulator/emulator.hpp>
#include <util/fs.hpp>

namespace emulator {

Emulator::Emulator(fs::path bios_path) : m_bios(bios_path), m_bus(m_bios), m_cpu(m_bus) {}

void Emulator::run() {
  bool cpu_halt;

  do {
    u32 cycles_passed;
    cpu_halt = m_cpu.step(cycles_passed);

  } while (!cpu_halt);
}

}  // namespace emulator
