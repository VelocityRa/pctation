#pragma once
#include <fstream>

#include <fs.hpp>
#include <types.hpp>

#include <array>
#include <memory>

static constexpr auto BIOS_SIZE = 512 * 1024;  // All BIOS images are 512KB

namespace bios {

class Bios {
 public:
  explicit Bios(fs::path const& path);
  u32 read32(u32 addr) const;

 private:
  std::unique_ptr<std::array<byte, BIOS_SIZE>> m_data;
};

}  // namespace bios
