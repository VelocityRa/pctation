#pragma once

#include <memory/map.hpp>
#include <util/types.hpp>

#include <array>
#include <memory>

namespace memory {

class Ram {
 public:
  Ram();
  u32 read32(u32 addr) const;
  void write32(u32 addr, u32 val) const;
  void write8(u32 addr, u8 val);

 private:
  std::unique_ptr<std::array<byte, RAM_SIZE>> m_data;
};

}  // namespace memory
