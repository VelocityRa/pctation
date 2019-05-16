#include <emulator/emulator.hpp>

#include <util/fs.hpp>

#include <algorithm>
#include <tuple>

namespace emulator {

Emulator::Emulator(const fs::path& bios_path,
                   const fs::path& psx_exe_path,
                   const fs::path& bootstrap_path,
                   const fs::path& cdrom_path)
    : m_bios(bios_path),
      m_expansion(bootstrap_path),
      m_interrupts(),
      m_ram(psx_exe_path),
      m_gpu(),
      m_spu(),
      m_cdrom(),
      m_dma(m_ram, m_gpu, m_interrupts, m_cdrom),
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

  if (!cdrom_path.empty())
    ;  // TODO:

  m_screen_renderer.set_texture_size(gpu::VRAM_WIDTH, gpu::VRAM_HEIGHT);
}

void Emulator::advance_frame() {
  // Run in 300 cycle chunks
  // Estimate that the CPU effectively runs at a 1/3 of the system clock (due to memory delays etc)
  const u32 system_cycle_quantum = 300;
  const u32 cpu_cycle_quantum = system_cycle_quantum / 3;

  while (true) {
    m_cpu.step(cpu_cycle_quantum);

    m_dma.step();
    m_cdrom.step();
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

void Emulator::set_view(View view) {
  switch (view) {
    case View::Display: {
      const auto res = m_gpu.get_resolution();
      m_settings.res_width = res.width;
      m_settings.res_height = res.height;
      break;
    }
    case View::Vram:
      m_settings.res_width = gpu::VRAM_WIDTH;
      m_settings.res_height = gpu::VRAM_HEIGHT;
      break;
  }

  m_screen_renderer.set_texture_size(m_settings.res_width, m_settings.res_height);
}

void Emulator::update_settings() {
  if (m_settings.window_size_changed) {
    set_view(m_settings.screen_view);
    // We leave screen_view_changed to true so that the GUI code can pick it up and change the window
    // size too
  }
}

}  // namespace emulator
