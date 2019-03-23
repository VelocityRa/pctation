#pragma once

#include <util/fs.hpp>
#include <util/types.hpp>

#include <array>
#include <memory>

namespace bios {

static constexpr u32 BIOS_SIZE = 512 * 1024;  // All BIOS images are 512KB

class Bios {
 public:
  explicit Bios(fs::path const& path);
  u32 read32(u32 addr) const;

 private:
  std::unique_ptr<std::array<byte, BIOS_SIZE>> m_data;
};

}  // namespace bios
