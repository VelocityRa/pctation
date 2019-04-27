#include <emulator/emulator.hpp>

#include <util/fs.hpp>

namespace emulator {

Emulator::Emulator(fs::path bios_path, fs::path psx_exe_path)
    : m_bios(bios_path),
      m_interrupts(),
      m_ram(psx_exe_path),
      m_gpu(),
      m_dma(m_ram, m_gpu),
      m_spu(),
      m_bus(m_bios, m_interrupts, m_scratchpad, m_ram, m_dma, m_gpu, m_spu),
      m_cpu(m_bus) {
  m_interrupts.init(&m_cpu);
}

void Emulator::advance_frame() {
  const u32 system_cycle_quantum = 300;
  const u32 cpu_cycle_quantum = system_cycle_quantum / 3;

  while (true) {
    // Run CPU
    m_cpu.step(cpu_cycle_quantum);

    // Run GPU
    if (m_gpu.step(system_cycle_quantum)) {
      m_bus.m_interrupts.trigger(cpu::IrqType::VBLANK);
      // Frame emulated, return to render it
      return;
    }
  }
}

void Emulator::draw() {
  m_gpu.draw();
}

}  // namespace emulator
