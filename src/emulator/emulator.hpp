#pragma once

#include <bios/bios.hpp>
#include <bus/bus.hpp>
#include <cpu/cpu.hpp>

#include <util/fs.hpp>

#include <optional>
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
