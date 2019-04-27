#include <cpu/interrupt.hpp>

#include <cpu/cpu.hpp>

#include <util/log.hpp>

namespace cpu {

void Interrupts::update_cop0() {
  const bool is_interrupt_pending = (m_imask.word & m_istat.word);

  m_cpu->m_cop0_cause.interrupt_pending = is_interrupt_pending ? 0b100 : 0b0;
}

void Interrupts::check() const {
  const auto cause = m_cpu->m_cop0_cause;
  const auto status = m_cpu->m_cop0_status;

  const bool are_interrupts_enabled = status.interrupt_enable;
  const bool are_active_interrupts = cause.interrupt_pending & status.interrupt_mask;

  if (are_interrupts_enabled && are_active_interrupts)
    m_cpu->trigger_exception(ExceptionCause::Interrupt);
}

void Interrupts::trigger(IrqType irq) {
  m_istat.word |= (1 << static_cast<u16>(irq));

  update_cop0();
}

}  // namespace cpu
