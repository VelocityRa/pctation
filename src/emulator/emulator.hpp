#pragma once

#include <bios/bios.hpp>
#include <bus/bus.hpp>
#include <cpu/cpu.hpp>
#include <gpu/gpu.hpp>
#include <memory/dma.hpp>
#include <memory/ram.hpp>

#include <util/fs.hpp>

namespace emulator {

constexpr u32 CPU_CYCLES_PER_SECOND = 33'868'800;
constexpr u32 FRAMERATE = 60;
constexpr u32 CPU_CYCLES_PER_FRAME = CPU_CYCLES_PER_SECOND / FRAMERATE;

class Emulator {
 public:
  explicit Emulator(fs::path bios_path);
  // Advances the emulator state approximately one frame
  void advance_frame();

  // Getters
  const cpu::Cpu& cpu() const { return m_cpu; }
  const memory::Ram& ram() const { return m_ram; }
  const gpu::Gpu& gpu() const { return m_gpu; }

 private:
  bios::Bios m_bios;
  memory::Ram m_ram;
  memory::Dma m_dma;
  gpu::Gpu m_gpu;
  bus::Bus m_bus;

  cpu::Cpu m_cpu;

  s32 m_cycles_left{ CPU_CYCLES_PER_FRAME };
};

}  // namespace emulator
