#pragma once

#include <bios/bios.hpp>
#include <bus/bus.hpp>
#include <cpu/cpu.hpp>
#include <gpu/gpu.hpp>
#include <memory/dma.hpp>
#include <memory/ram.hpp>

#include <util/fs.hpp>

namespace emulator {

class Emulator {
 public:
  explicit Emulator(fs::path bios_path);
  void run();

 private:
  bios::Bios m_bios;
  memory::Ram m_ram;
  memory::Dma m_dma;
  gpu::Gpu m_gpu;
  bus::Bus m_bus;

  cpu::Cpu m_cpu;
};

}  // namespace emulator
