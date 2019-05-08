#include <io/cdrom.hpp>

#include <util/log.hpp>

#include <gsl-lite.hpp>

namespace io {

void Cdrom::init(cpu::Interrupts* interrupts) {
  m_interrupts = interrupts;
}

u8 Cdrom::read_reg(address addr_rebased) {
  const u8 reg = addr_rebased;
  u8 val = 0;

  if (reg == 0) {                                           // Status Register
  } else if (reg == 1 && m_index == 0) {                    // Command Register
  } else if (reg == 1) {                                    // Response FIFO
  } else if (reg == 2) {                                    // Data FIFO
  } else if (reg == 3 && (m_index == 0 || m_index == 2)) {  // Interrupt Enable Register
  } else if (reg == 3 && (m_index == 1 || m_index == 3)) {  // Interrupt Flag Register
  } else {
    LOG_CRITICAL("Unknown combination, CDREG{}.{}", reg, m_index);
  }

  LOG_ERROR("CDROM read CDREG{}.{} val: 0x{:02X} ({:#010b})", reg, m_index, val, val);

  return 0;
}

void Cdrom::write_reg(address addr_rebased, u8 val) {
  const u8 reg = addr_rebased;

  LOG_ERROR("CDROM write CDREG{}.{} val: 0x{:02X} ({:#010b})", reg, m_index, val, val);

  if (reg == 0) {  // Index Register
    m_index = val & 0b11;
  } else if (reg == 1 && m_index == 0) {  // Command Register
  } else if (reg == 1 && m_index == 1) {  // Sound Map Data Out
  } else if (reg == 1 && m_index == 2) {  // Sound Map Coding Info
  } else if (reg == 1 && m_index == 3) {  // Audio Volume for Right-CD-Out to Right-SPU-Input
  } else if (reg == 2 && m_index == 0) {  // Parameter FIFO
  } else if (reg == 2 && m_index == 1) {  // Interrupt Enable Register
  } else if (reg == 2 && m_index == 2) {  // Audio Volume for Left-CD-Out to Left-SPU-Input
  } else if (reg == 2 && m_index == 3) {  // Audio Volume for Right-CD-Out to Left-SPU-Input
  } else if (reg == 3 && m_index == 0) {  // Request Register
  } else if (reg == 3 && m_index == 1) {  // Interrupt Flag Register
  } else if (reg == 3 && m_index == 2) {  // Audio Volume for Left-CD-Out to Right-SPU-Input
  } else if (reg == 3 && m_index == 3) {  // Audio Volume Apply Changes
  } else {
    LOG_CRITICAL("Unknown combination, CDREG{}.{} val: {:02X}", reg, m_index, val);
  }
}

}  // namespace io
