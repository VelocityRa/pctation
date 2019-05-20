#pragma once

#include <util/types.hpp>

namespace io {

class DigitalController {
 public:
  void update_button(u8 button_index, bool was_pressed);
  u8 read(u8 val);
  bool ack() const { return m_read_idx != 0; }  // Does NOT get triggered after receiving the LAST byte

  void advance() { ++m_read_idx; }
  void reset() { m_read_idx = 0; }

  u8 m_read_idx{};  // Which byte in the communication sequence is being read
  union {
    u16 word{ 0xFFFF };
    u8 byte[2];
  } m_buttons{};

  // TODO: This does prevent some presses from getting lost, but not consecutive ones
  union {
    u16 word;
    u8 byte[2];
  } m_buttons_down_mask;
};

}  // namespace io
