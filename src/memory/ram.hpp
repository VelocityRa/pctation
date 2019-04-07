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
  u16 read16(u32 addr);
  u8 read8(u32 addr) const;
  void write32(u32 addr, u32 val);
  void write16(u32 addr, u16 val);
  void write8(u32 addr, u8 val);
  const std::array<byte, RAM_SIZE>& data() const { return *m_data; }

 private:
  std::unique_ptr<std::array<byte, RAM_SIZE>> m_data;
};

}  // namespace memory
