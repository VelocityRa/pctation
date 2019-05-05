#pragma once

#include <io/digital_controller.hpp>
#include <util/types.hpp>

#include <memory/range.hpp>

namespace cpu {
class Interrupts;
}

namespace io {

// Addresses are off-set from Joypad base (0x1F801040)
static constexpr memory::Range JOY_DATA{ 0x0, 4 };
static constexpr memory::Range JOY_STAT{ 0x4, 4 };
static constexpr memory::Range JOY_MODE{ 0x8, 2 };
static constexpr memory::Range JOY_CTRL{ 0xA, 2 };
static constexpr memory::Range JOY_BAUD{ 0xE, 2 };

static constexpr u8 BTN_INVALID = 0xFF;
static constexpr u8 BTN_SELECT = 0;
static constexpr u8 BTN_L3 = 1;
static constexpr u8 BTN_R3 = 2;
static constexpr u8 BTN_START = 3;
static constexpr u8 BTN_PAD_UP = 4;
static constexpr u8 BTN_PAD_RIGHT = 5;
static constexpr u8 BTN_PAD_DOWN = 6;
static constexpr u8 BTN_PAD_LEFT = 7;
static constexpr u8 BTN_L2 = 8;
static constexpr u8 BTN_R2 = 9;
static constexpr u8 BTN_L1 = 10;
static constexpr u8 BTN_R1 = 11;
static constexpr u8 BTN_TRIANGLE = 12;
static constexpr u8 BTN_CIRCLE = 13;
static constexpr u8 BTN_CROSS = 14;
static constexpr u8 BTN_SQUARE = 15;

class Joypad {
 public:
  void init(cpu::Interrupts* interrupts);
  u8 read8(address addr_rebased);
  void write8(address addr_rebased, u8 val);

  void step();
  void update_button(u8 button_index, bool was_pressed);

  static const char* addr_to_reg_name(address addr_rebased);

 private:
  void do_tx_transfer(u8 val);

 private:
  enum class Device {
    None,
    Controller,
    MemoryCard,
  };

  u16 m_reg_mode{};
  u16 m_reg_ctrl{};
  u16 m_reg_baud{};

  bool m_rx_has_data{};
  u8 m_rx_data{};

  bool m_irq{};
  u8 m_irq_timer{};
  bool m_ack{};

  Device m_device_selected{ Device::None };

  DigitalController m_digital_controllers[2];

  cpu::Interrupts* m_interrupts;
};

}  // namespace io
