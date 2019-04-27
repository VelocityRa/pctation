#pragma once

#include <bios/bios.hpp>
#include <bus/bus.hpp>
#include <cpu/cpu.hpp>
#include <cpu/interrupt.hpp>
#include <gpu/gpu.hpp>
#include <memory/dma.hpp>
#include <memory/ram.hpp>
#include <spu/spu.hpp>

#include <util/fs.hpp>

namespace emulator {

class Emulator {
 public:
  explicit Emulator(fs::path bios_path, fs::path psx_exe_path);
  // Advances the emulator state approximately one frame
  void advance_frame();

  // Getters
  const cpu::Cpu& cpu() const { return m_cpu; }
  const memory::Ram& ram() const { return m_ram; }
  const gpu::Gpu& gpu() const { return m_gpu; }
  void draw();

 private:
  bios::Bios m_bios;
  cpu::Interrupts m_interrupts;
  memory::Scratchpad m_scratchpad;
  memory::Ram m_ram;
  gpu::Gpu m_gpu;
  spu::Spu m_spu;
  memory::Dma m_dma;
  bus::Bus m_bus;

  cpu::Cpu m_cpu;
};

}  // namespace emulator
