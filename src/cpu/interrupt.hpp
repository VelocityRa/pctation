#pragma once

#include <memory/addressable.hpp>
#include <util/types.hpp>

namespace cpu {

class Cpu;

enum class IrqType : u16 {
  VBLANK = 0,
  GPU = 1,
  CDROM = 2,
  DMA = 3,
  TIMER0 = 4,
  TIMER1 = 5,
  TIMER2 = 6,
  CONTROLLER = 7,  // also memory card
  SIO = 8,
  SPU = 9,
};

union Irq {
  struct {
    IrqType vblank : 1;
    IrqType gpu : 1;
    IrqType cdrom : 1;
    IrqType dma : 1;
    IrqType timer0 : 1;
    IrqType timer1 : 1;
    IrqType timer2 : 1;
    IrqType controller : 1;
    IrqType sio : 1;
    IrqType spu : 1;
  };
  u16 word;
  u8 byte[2];
};

class Interrupts {
 public:
  void init(cpu::Cpu* cpu) { m_cpu = cpu; }

  void update_cop0();
  void check() const;
  void trigger(IrqType irq);

  template <typename ValueType>
  ValueType read(address addr_rebased) const {
    const u16* reg = nullptr;
    address reg_addr = addr_rebased;

    if (addr_rebased < 2) {
      reg = &m_istat.word;
      reg_addr = addr_rebased;
    } else if (addr_rebased >= 4 && addr_rebased < 6) {
      reg = &m_imask.word;
      reg_addr = addr_rebased - 4;
    }

    return *(ValueType*)((u8*)reg + reg_addr);
  }
  template <typename ValueType>
  void write(address addr_rebased, ValueType val) {
    if (addr_rebased < 2) {
      *(ValueType*)((u8*)&m_istat.word + addr_rebased) &= val;
    } else if (addr_rebased >= 4 && addr_rebased < 6) {
      *(ValueType*)((u8*)&m_imask.word + addr_rebased - 4) = val;
    }

    update_cop0();
  }

 private:
  Irq m_istat;
  Irq m_imask;

  cpu::Cpu* m_cpu;
};

}  // namespace cpu
