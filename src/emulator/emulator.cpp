#include <emulator/emulator.hpp>

#include <util/fs.hpp>

#include <tuple>

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
  m_screen_renderer.set_texture_size(gpu::VRAM_WIDTH, gpu::VRAM_HEIGHT);
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
  m_screen_renderer.render((const void*)m_gpu.vram().data());
}

WindowSize Emulator::toggle_view() {
  // Advance View enum by 1 and wrap
  m_view = View(((u8)m_view + 1) % (u8)View::Maximum);

  WindowSize new_size{};

  switch (m_view) {
    case View::Display:
      new_size.width = 640;  // TODO: get from m_gpu
      new_size.height = 480;
      break;

    case View::Vram:
      new_size.width = gpu::VRAM_WIDTH;
      new_size.height = gpu::VRAM_HEIGHT;
      break;
  }

  m_screen_renderer.set_texture_size(new_size.width, new_size.height);

  return new_size;
}

}  // namespace emulator
