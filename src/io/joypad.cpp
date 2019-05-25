#include <io/joypad.hpp>

#include <cpu/interrupt.hpp>
#include <util/log.hpp>

namespace io {

void Joypad::init(cpu::Interrupts* interrupts) {
  m_interrupts = interrupts;
}

u8 Joypad::read8(address addr_rebased) {
  u32 reg_byte;

  if (JOY_DATA.contains(addr_rebased, reg_byte)) {
    if (reg_byte == 0) {
      auto rx_data = m_rx_data;
      // TODO: should we check has_data first?
      m_rx_data = 0xFF;
      m_rx_has_data = false;
      return rx_data;
    } else
      return 0x00;  // Preview unimplemented
  }
  if (JOY_STAT.contains(addr_rebased, reg_byte)) {
    if (reg_byte == 0) {
      u8 stat_0 = 0b101;             // Set TX Ready flags to 1 (bits 0 and 2)
      stat_0 |= m_rx_has_data << 1;  // Set RX FIFO not empty (bit 1)
      stat_0 |= m_ack << 7;          // Set /ACK Input Level
      // The of the bits are 0
      m_ack = false;
      return stat_0;
    } else if (reg_byte == 1) {
      return (m_irq << 1);
    } else {
      LOG_WARN_JOYPAD("JOY_STAT unimplemented byte read from");
      return 0;
    }
  }
  if (JOY_MODE.contains(addr_rebased, reg_byte))
    return *((u8*)&m_reg_mode + reg_byte);
  if (JOY_CTRL.contains(addr_rebased, reg_byte))
    return *((u8*)&m_reg_ctrl + reg_byte);
  if (JOY_BAUD.contains(addr_rebased, reg_byte))
    return *((u8*)&m_reg_baud + reg_byte);

  // Unreachable
  assert(0);
  return 0xFF;
}

void Joypad::write8(address addr_rebased, u8 val) {
  u32 reg_byte;

  if (JOY_DATA.contains(addr_rebased, reg_byte)) {
    do_tx_transfer(val);
    return;
  } else if (JOY_STAT.contains(addr_rebased, reg_byte)) {
    LOG_WARN_JOYPAD("Unhandled JOY_STAT[{}] write of {:02}", reg_byte, val);
  } else if (JOY_MODE.contains(addr_rebased, reg_byte))
    *((u8*)&m_reg_mode + reg_byte) = val;
  else if (JOY_CTRL.contains(addr_rebased, reg_byte)) {
    *((u8*)&m_reg_ctrl + reg_byte) = val;

    if (reg_byte == 0 && (val & 0x10))
      m_irq = false;  // reset IRQ

    if (reg_byte == 1) {
      if (!(m_reg_ctrl & 0b10)) {
        m_device_selected = Device::None;
        for (auto c : m_digital_controllers)
          c.reset();
      }
    }
  } else if (JOY_BAUD.contains(addr_rebased, reg_byte))
    *((u8*)&m_reg_baud + reg_byte) = val;
}

void Joypad::step() {
  if (m_irq_timer > 0) {
    m_irq_timer--;
    if (m_irq_timer == 0) {
      m_irq = true;  // trigger IRQ
      m_ack = false;
    }
  }

  if (m_irq)
    m_interrupts->trigger(cpu::IrqType::CONTROLLER);
}

void Joypad::update_button(u8 button_index, bool was_pressed) {
  // TODO: Player 2 support
  m_digital_controllers[0].update_button(button_index, was_pressed);
}

const char* Joypad::addr_to_reg_name(address addr_rebased) {
  address reg_byte;

  if (JOY_DATA.contains(addr_rebased, reg_byte))
    return "JOY_DATA";
  if (JOY_STAT.contains(addr_rebased, reg_byte))
    return "JOY_STAT";
  if (JOY_MODE.contains(addr_rebased, reg_byte))
    return "JOY_MODE";
  if (JOY_CTRL.contains(addr_rebased, reg_byte))
    return "JOY_CTRL";
  if (JOY_BAUD.contains(addr_rebased, reg_byte))
    return "JOY_BAUD";

  return "<unknown>";
}

void Joypad::do_tx_transfer(u8 val) {
  m_rx_has_data = true;

  u8 port = (m_reg_ctrl >> 13) & 0b1;

  if (m_device_selected == Device::None) {
    if (val == 0x01) {
      m_device_selected = Device::Controller;
    } else if (val == 0x81) {
      m_device_selected = Device::MemoryCard;
    }
  }

  // Read from seleted device immediately

  if (m_device_selected == Device::Controller) {
    m_rx_data = m_digital_controllers[port].read(val);
    m_ack = m_digital_controllers[port].ack();
    if (m_ack)
      m_irq_timer = 5;
    if (m_digital_controllers[port].m_read_idx == 0)
      m_device_selected = Device::None;
  }

  if (m_device_selected == Device::MemoryCard) {
    LOG_WARN_JOYPAD("Requested read from Memory Card, unimplemented");
    m_device_selected = Device::None;
    m_rx_data = 0xFF;
    m_ack = true;
  }
}

}  // namespace io
