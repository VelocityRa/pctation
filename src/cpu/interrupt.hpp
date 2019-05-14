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
    u16 vblank : 1;
    u16 gpu : 1;
    u16 cdrom : 1;
    u16 dma : 1;
    u16 timer0 : 1;
    u16 timer1 : 1;
    u16 timer2 : 1;
    u16 controller : 1;
    u16 sio : 1;
    u16 spu : 1;
  };
  u16 word;
  u8 byte[2];
};

class Interrupts {
 public:
  void init(cpu::Cpu* cpu) { m_cpu = cpu; }

  bool check() const;
  void update_cop0();
  void check_and_trigger() const;
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

    return *(ValueType*)((u8*)reg + reg_addr) & 0x7FF;
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
