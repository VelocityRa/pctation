#include <emulator/emulator.hpp>

#include <util/fs.hpp>

namespace emulator {

Emulator::Emulator(fs::path bios_path, fs::path psx_exe_path)
    : m_bios(bios_path),
      m_ram(psx_exe_path),
      m_gpu(),
      m_dma(m_ram, m_gpu),
      m_bus(m_bios, m_ram, m_dma, m_gpu),
      m_cpu(m_bus) {}

void Emulator::advance_frame() {
  //  m_cycles_left += CPU_CYCLES_PER_FRAME;

  //  while (m_cycles_left > 0) {
  while (true) {
    u32 cycles_passed;

    if (m_cpu.step(cycles_passed))
      break;

    // HACK: This is for BIOS, end the frame here
    if (m_gpu.trigger_frame())
      break;

    //    m_cycles_left -= cycles_passed;
  }
}

void Emulator::draw() {
  m_gpu.draw();
}

}  // namespace emulator
