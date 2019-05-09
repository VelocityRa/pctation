#include <emulator/emulator.hpp>

#include <util/fs.hpp>

namespace emulator {

Emulator::Emulator(fs::path bios_path, fs::path psx_exe_path, fs::path bootstrap_path)
    : m_bios(bios_path),
      m_expansion(bootstrap_path),
      m_interrupts(),
      m_ram(psx_exe_path),
      m_gpu(),
      m_spu(),
      m_dma(m_ram, m_gpu, m_interrupts),
      m_bus(m_bios,
            m_expansion,
            m_interrupts,
            m_scratchpad,
            m_ram,
            m_dma,
            m_gpu,
            m_spu,
            m_joypad,
            m_cdrom),
      m_cpu(m_bus) {
  m_interrupts.init(&m_cpu);
  m_joypad.init(&m_interrupts);
  m_cdrom.init(&m_interrupts);
}

void Emulator::advance_frame() {
  const u32 system_cycle_quantum = 300;
  const u32 cpu_cycle_quantum = system_cycle_quantum / 3;

  while (true) {
    m_cpu.step(cpu_cycle_quantum);

    m_dma.step();
    m_joypad.step();

    if (m_gpu.step(system_cycle_quantum)) {
      m_bus.m_interrupts.trigger(cpu::IrqType::VBLANK);
      // Frame emulated, return to render it
      return;
    }
  }
}

void Emulator::render() {
  m_screen_renderer.render(gpu::VRAM_WIDTH, gpu::VRAM_HEIGHT, (const void*)m_gpu.vram().data());
}

}  // namespace emulator
