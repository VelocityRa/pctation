#pragma once

#include <bios/bios.hpp>
#include <bus/bus.hpp>
#include <cpu/cpu.hpp>
#include <cpu/interrupt.hpp>
#include <emulator/settings.hpp>
#include <gpu/gpu.hpp>
#include <io/cdrom_drive.hpp>
#include <io/joypad.hpp>
#include <io/timers.hpp>
#include <memory/dma.hpp>
#include <memory/expansion.hpp>
#include <memory/ram.hpp>
#include <renderer/screen_renderer.hpp>
#include <spu/spu.hpp>

#include <util/fs.hpp>

namespace gui {
class Gui;
}

namespace emulator {

class Emulator {
 public:
  explicit Emulator(const fs::path& bios_path,
                    const fs::path& psx_exe_path,
                    const fs::path& bootstrap_path,
                    const fs::path& cdrom_path);

  // Advances the emulator state approximately one frame
  void advance_frame();
  void render();
  void set_view(View view);

  // Getters
  const cpu::Cpu& cpu() const { return m_cpu; }
  const memory::Ram& ram() const { return m_ram; }
  const gpu::Gpu& gpu() const { return m_gpu; }
  io::Joypad& joypad() { return m_joypad; }
  const io::Timers& timers() const { return m_timers; }
  Settings& settings() { return m_settings; }
  void update_settings();

 private:
  // Emulator core components
  bios::Bios m_bios;
  memory::Expansion m_expansion;
  cpu::Interrupts m_interrupts;
  memory::Scratchpad m_scratchpad;
  memory::Ram m_ram;
  gpu::Gpu m_gpu;
  spu::Spu m_spu;
  io::Joypad m_joypad;
  io::CdromDrive m_cdrom;
  io::Timers m_timers;
  memory::Dma m_dma;
  bus::Bus m_bus;

  cpu::Cpu m_cpu;

 private:
  // Host fields
  renderer::ScreenRenderer m_screen_renderer;
  emulator::Settings m_settings{};
};

}  // namespace emulator
