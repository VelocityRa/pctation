#include <memory/ram.hpp>

#include <algorithm>
#include <array>

namespace memory {

Ram::Ram() {
  m_data = std::make_unique<std::array<byte, memory::RAM_SIZE>>();
  // Fill with specific data for debugging
  std::fill(m_data->begin(), m_data->end(), 0xDE);
}

u32 Ram::read32(u32 addr) const {
  return *(u32*)(m_data.get()->data() + addr);
}

void Ram::write32(u32 addr, u32 val) const {
  *(u32*)(m_data.get()->data() + addr) = val;
}

void Ram::write8(u32 addr, u8 val) {
  m_data.get()->data()[addr] = val;
}

}  // namespace memory
