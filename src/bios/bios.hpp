#pragma once

#include <memory/map.hpp>
#include <util/fs.hpp>
#include <util/types.hpp>

#include <array>
#include <memory>

namespace bios {

class Bios {
 public:
  explicit Bios(fs::path const& path);
  u32 read32(u32 addr) const;
  u8 read8(u32 addr) const;

 private:
  std::unique_ptr<std::array<byte, memory::BIOS_SIZE>> m_data;
};

}  // namespace bios
