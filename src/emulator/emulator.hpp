#pragma once

#include <bios/bios.hpp>
#include <bus/bus.hpp>
#include <cpu/cpu.hpp>
#include <cpu/interrupt.hpp>
#include <gpu/gpu.hpp>
#include <io/cdrom.hpp>
#include <io/joypad.hpp>
#include <memory/dma.hpp>
#include <memory/expansion.hpp>
#include <memory/ram.hpp>
#include <renderer/screen_renderer.hpp>
#include <spu/spu.hpp>

#include <util/fs.hpp>

namespace emulator {

enum class View : u8 {
  Display,  // Show only the display output that would be shown on a real console
  Vram,     // Show the contents of the entire VRAM

  Maximum,
};

struct WindowSize {
  s32 width;
  s32 height;
};

class Emulator {
 public:
  explicit Emulator(fs::path bios_path, fs::path psx_exe_path, fs::path bootstrap_path);

  // Advances the emulator state approximately one frame
  void advance_frame();
  void render();
  WindowSize toggle_view();

  // Getters
  const cpu::Cpu& cpu() const { return m_cpu; }
  const memory::Ram& ram() const { return m_ram; }
  const gpu::Gpu& gpu() const { return m_gpu; }
  io::Joypad& joypad() { return m_joypad; }

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
  io::Cdrom m_cdrom;
  memory::Dma m_dma;
  bus::Bus m_bus;

  cpu::Cpu m_cpu;

 private:
  // Host fields
  renderer::ScreenRenderer m_screen_renderer;
  emulator::View m_view = View::Vram;
};

}  // namespace emulator
