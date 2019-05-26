#include <cpu/interrupt.hpp>
#include <gsl-lite.hpp>
#include <io/timers.hpp>
#include <util/log.hpp>

namespace io {

static cpu::IrqType timer_index_to_irq(TimerIndex i);

void Timers::init(cpu::Interrupts* interrupts) {
  m_interrupts = interrupts;
}

void Timers::step(u32 cycles) {
  const u16 timer_increment[3] = { source0() ? cycles : cycles,  // TODO
                                   source1() ? cycles : cycles,  // TODO
                                   source2() ? cycles / 8 : cycles };

  for (auto i = Timer0; i < TimerMax; i = (TimerIndex)((u16)i + 1)) {
    if (m_timer_paused[i])
      continue;

    // Shorthands
    auto& value = m_timer_value[i];
    auto& mode = m_timer_mode[i];
    auto& target = m_timer_target[i];

    value += timer_increment[i];

    bool could_irq = false;

    if (value > target) {
      mode.reached_target = true;
      if (mode.irq_on_target)
        could_irq = true;
      if (mode.reset_on_target)
        value = 0;  // if reset on target
    }

    if (value > 0xFFFF) {
      mode.reached_max = true;
      if (mode.irq_on_max)
        could_irq = true;
      if (!mode.reset_on_target)
        value = 0;  // if reset on max
    }

    if (could_irq)
      step_irq(i);

    // Handle possible overflow (we use u32 but counter value is u16)
    value = static_cast<u16>(value);
  }
}

u16 Timers::read_reg(address addr) {
  u8 timer_select = timer_from_addr(addr);
  u8 reg = addr & 0xF;

  switch (reg) {
    case 0:  // Current Counter Value
      return static_cast<u16>(m_timer_value[timer_select]);
    case 4: {                                     // Counter Mode
      m_timer_irq_occured[timer_select] = false;  // Reset one-shot IRQ tracker
      m_timer_value[timer_select] = 0;
      return m_timer_mode[timer_select].read();
    }
    case 8:  // Counter Target Value
      return m_timer_target[timer_select];
    default: LOG_ERROR("Invalid Timer register access"); return 0xFFFF;
  }
}

void Timers::write_reg(address addr, u16 val) {
  u8 timer_select = timer_from_addr(addr);
  u8 reg = addr & 0xF;

  // Shorthands
  auto& value = m_timer_value[timer_select];
  auto& mode = m_timer_mode[timer_select];
  auto& target = m_timer_target[timer_select];

  switch (reg) {
    case 0:  // Current Counter Value
      value = static_cast<u16>(val);
      break;
    case 4:  // Counter Mode
      mode.word = val;
      mode.irq_not = true;

      m_timer_paused[timer_select] = false;
      m_timer_irq_occured[timer_select] = false;

      value = 0;

      if (mode.sync_enable) {
        if (timer_select == 2) {  // TODO: other sync modes
          if (mode.sync_mode == 0 || mode.sync_mode == 3)
            m_timer_paused[timer_select] = true;
        }
      }
      break;
    case 8:  // Counter Target Value
      target = val;
      break;
    default: LOG_ERROR("Invalid Timer register access"); break;
  }
}

void Timers::step_irq(TimerIndex i) {
  auto& mode = m_timer_mode[i];

  if (mode.irq_toggle_mode() == TimerMode::ToggleMode::Toggle)
    mode.irq_not ^= 1;
  else
    mode.irq_not = false;

  if (mode.irq_repeat_mode() == TimerMode::RepeatMode::Once && m_timer_irq_occured[i])
    return;

  if (!mode.irq_not) {
    m_interrupts->trigger(timer_index_to_irq(i));
    m_timer_irq_occured[i] = true;
  }
  mode.irq_not = true;
}

u8 Timers::timer_from_addr(address addr) {
  const auto timer_select = (addr & 0xF0) >> 4;
  Expects(timer_select <= 2);
  return timer_select;
}

bool Timers::source0() const {
  return m_timer_mode[0].clock_source & 0b10;
}
bool Timers::source1() const {
  return m_timer_mode[1].clock_source & 0b10;
}
bool Timers::source2() const {
  return m_timer_mode[2].clock_source >= 2;
}

static cpu::IrqType timer_index_to_irq(TimerIndex i) {
  switch (i) {
    case 0: return cpu::IrqType::TIMER0;
    case 1: return cpu::IrqType::TIMER1;
    case 2: return cpu::IrqType::TIMER2;
    default: LOG_CRITICAL("Invalid interrupt"); return cpu::IrqType::INVALID;
  }
}

}  // namespace io
