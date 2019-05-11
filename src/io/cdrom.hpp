#pragma once

#include <deque>
#include <initializer_list>
#include <util/types.hpp>
#include <vector>

namespace cpu {
class Interrupts;
}

namespace io {

constexpr size_t MAX_FIFO_SIZE = 16;

enum CdromResponseType : u8 {
  NoneInt0 = 0,     // INT0: No response received (no interrupt request)
  SecondInt1 = 1,   // INT1: Received SECOND (or further) response to ReadS/ReadN (and Play+Report)
  SecondInt2 = 2,   // INT2: Received SECOND response (to various commands)
  FirstInt3 = 3,    // INT3: Received FIRST response (to any command)
  DataEndInt4 = 4,  // INT4: DataEnd (when Play/Forward reaches end of disk) (maybe also for Read?)
  ErrorInt5 = 5,    // INT5: Received error-code (in FIRST or SECOND response)
};

enum class CdromReadState {
  Init,
  Seeking,
  Playing,
  Reading,
};

union CdromStatusRegister {
  u8 byte{};

  struct {
    u8 index : 2;
    u8 adpcm_fifo_empty : 1;
    u8 param_fifo_empty : 1;
    u8 param_fifo_write_ready : 1;
    u8 response_fifo_not_empty : 1;
    u8 data_fifo_empty : 1;
    u8 transmit_busy : 1;
  };

  CdromStatusRegister() {
    param_fifo_empty = true;
    param_fifo_write_ready = true;
  }
};

union CdromMode {
  u8 byte{};

  struct {
    u8 cd_da_read : 1;   // (0=Off, 1=Allow to Read CD-DA Sectors; ignore missing EDC)
    u8 auto_pause : 1;   // (0=Off, 1=Auto Pause upon End of Track) ;for Audio Play
    u8 report : 1;       // (0=Off, 1=Enable Report-Interrupts for Audio Play)
    u8 xa_filter : 1;    // (0=Off, 1=Process only XA-ADPCM sectors that match Setfilter)
    u8 ignore_bit : 1;   // (0=Normal, 1=Ignore Sector Size and Setloc position)
    u8 sector_size : 1;  // (0=800h=DataOnly, 1=924h=WholeSectorExceptSyncBytes)
    u8 xa_adpcm : 1;     // (0=Off, 1=Send XA-ADPCM sectors to SPU Audio Input)
    u8 speed : 1;        // (0=Normal speed, 1=Double speed)
  };

  void reset() { byte = 0; }
};

union CdromStatusCode {
  u8 byte{};

  struct {
    u8 error : 1;
    u8 spindle_motor_on : 1;
    u8 seek_error : 1;
    u8 id_error : 1;
    u8 shell_open : 1;
    u8 reading : 1;
    u8 seeking : 1;
    u8 playing : 1;
  };

  CdromStatusCode() { shell_open = true; }

  // Does not reset shell open state
  void reset() { error = spindle_motor_on = seek_error = id_error = reading = seeking = playing = 0; }

  void set_state(CdromReadState state) {
    reset();
    spindle_motor_on = true;  // Turn on motor
    switch (state) {
      case CdromReadState::Seeking: seeking = true; break;
      case CdromReadState::Playing: playing = true; break;
      case CdromReadState::Reading: reading = true; break;
    }
  }
};

class Cdrom {
 public:
  void init(cpu::Interrupts* interrupts);
  void step();
  u8 read_reg(address addr_rebased);
  void write_reg(address addr_rebased, u8 val);

 private:
  void execute_command(u8 cmd);
  void push_response(CdromResponseType type, std::initializer_list<u8> bytes);
  void push_response(CdromResponseType type, u8 byte);
  void command_error();
  u8 get_param();

  static const char* get_reg_name(u8 reg, u8 index, bool is_read);
  static const char* get_cmd_name(u8 cmd);

 private:
  CdromStatusRegister m_reg_status{};
  CdromStatusCode m_status_code{};
  CdromMode m_mode{};
  CdromReadState m_read_state{};

  u32 m_seek_loc{};
  u32 m_read_loc{};

  std::vector<u8> m_param_fifo{};
  std::deque<CdromResponseType> m_irq_fifo{};
  std::deque<u8> m_resp_fifo{};

  u8 m_reg_int_enable{};

  cpu::Interrupts* m_interrupts{};
};

}  // namespace io
