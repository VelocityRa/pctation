#pragma once

#include <util/types.hpp>

namespace cpu {
class Interrupts;
}

namespace gui {
class Gui;
}

namespace io {

union TimerMode {
  u16 word{};

  struct {
    u16 sync_enable : 1;      // [0]    Synchronization Enable (0=Free Run, 1=Synchronize via Bit1-2)
    u16 sync_mode : 2;        // [1-2]  Synchronization Mode   (0-3)
    u16 reset_on_target : 1;  // [3]    Reset counter to 0000h  (0=After Counter=FFFFh, 1=After
                              // Counter=Target)
    u16 irq_on_target : 1;    // [4]    IRQ when Counter=Target (0=Disable, 1=Enable)
    u16 irq_on_max : 1;       // [5]    IRQ when Counter=FFFFh  (0=Disable, 1=Enable)
    u16 irq_repeat_mode : 1;  // [6]    IRQ Once/Repeat Mode    (0=One-shot, 1=Repeatedly)
    u16 irq_toggle_mode : 1;  // [7]    IRQ Pulse/Toggle Mode   (0=Short Bit10=0 Pulse, 1=Toggle Bit10
                              // on/off)
    u16 clock_source : 2;     // [8-9]  Clock Source (0-3)
    u16 irq_not : 1;  // [10]   Interrupt Request       (0=Yes, 1=No) (Set after Writing) (W=1) (R)
    u16 reached_target : 1;  // [11]   Reached Target Value    (0=No, 1=Yes) (Reset after Reading) (R)
    u16 reached_max : 1;     // [12]   Reached FFFFh Value     (0=No, 1=Yes) (Reset after Reading) (R)
  };

  u16 read() {
    auto reg_val = word;
    reached_target = false;
    reached_max = false;
    return reg_val;
  }
};

enum TimerIndex : u16 {
  Timer0 = 0,
  Timer1 = 1,
  Timer2 = 2,

  TimerMax,
};

class Timers {
  friend class gui::Gui;  // for debug info

 public:
  void init(cpu::Interrupts* interrupts);
  void step(u32 cpu_cycles);

  u16 read_reg(address addr);
  void write_reg(address addr, u16 val);

 private:
  void step_irq(TimerIndex i);  // Returns whether an IRQ should occur
  static u8 timer_from_addr(address addr);

  bool source0() const;
  bool source1() const;
  bool source2() const;

  bool timer2_paused() const;

 private:
  cpu::Interrupts* m_interrupts{};

  u32 m_timer_value[3]{};  // Use u32 instead of u16 to handle u16 overflow
  TimerMode m_timer_mode[3]{};
  u16 m_timer_target[3]{};

  bool m_timer_irq_occured[3]{};  // Set when a one-shot IRQ happens
};

}  // namespace io
