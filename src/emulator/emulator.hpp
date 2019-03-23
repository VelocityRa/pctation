#pragma once

#include <bios.hpp>
#include <bus.hpp>
#include <cpu.hpp>

#include <fs.hpp>

namespace emulator {

class Emulator {
 public:
  explicit Emulator(fs::path bios_path);
  void run();

 private:
  bios::Bios m_bios;
  bus::Bus m_bus;

  cpu::Cpu m_cpu;
};

}  // namespace emulator
