#include <memory/ram.hpp>

#include <algorithm>
#include <array>

namespace memory {

Ram::Ram() {
  m_data = std::make_unique<std::array<byte, memory::RAM_SIZE>>();
  m_data_scratch = std::make_unique<std::array<byte, memory::SCRATCHPAD_SIZE>>();

  // Fill with specific byte for debugging
  std::fill(m_data->begin(), m_data->end(), 0xDE);
  std::fill(m_data_scratch->begin(), m_data_scratch->end(), 0xDE);
}

u32 Ram::read32(u32 addr) const {
  return *(u32*)(m_data.get()->data() + addr);
}

u16 Ram::read16(u32 addr) {
  return *(u16*)(m_data.get()->data() + addr);
}

u8 Ram::read8(u32 addr) const {
  return m_data.get()->data()[addr];
}

void Ram::write32(u32 addr, u32 val) {
  *(u32*)(m_data.get()->data() + addr) = val;
}

void Ram::write16(u32 addr, u16 val) {
  *(u16*)(m_data.get()->data() + addr) = val;
}

void Ram::write8(u32 addr, u8 val) {
  m_data.get()->data()[addr] = val;
}

// TODO: refactor
u32 Ram::read32_scratch(u32 addr) const {
  return *(u32*)(m_data_scratch.get()->data() + addr);
}
u16 Ram::read16_scratch(u32 addr) {
  return *(u16*)(m_data_scratch.get()->data() + addr);
}
u8 Ram::read8_scratch(u32 addr) const {
  return m_data_scratch.get()->data()[addr];
}
void Ram::write32_scratch(u32 addr, u32 val) {
  *(u32*)(m_data_scratch.get()->data() + addr) = val;
}
void Ram::write16_scratch(u32 addr, u16 val) {
  *(u16*)(m_data_scratch.get()->data() + addr) = val;
}
void Ram::write8_scratch(u32 addr, u8 val) {
  m_data_scratch.get()->data()[addr] = val;
}

}  // namespace memory
