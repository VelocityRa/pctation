#pragma once

#include <io/cdrom_disk.hpp>
#include <util/fs.hpp>
#include <util/types.hpp>

#include <deque>
#include <initializer_list>

namespace cpu {
class Interrupts;
}

namespace io {

constexpr auto READ_SECTOR_DELAY_STEPS = 1150;  // IRQ delay in CD-ROM steps (each one is 100 CPU cycles)
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
  Stopped,
  Seeking,
  Playing,
  Reading,
};

union CdromStatusRegister {
  u8 byte{};

  struct {
    u8 index : 2;
    u8 adpcm_fifo_empty : 1;         // set when playing XA-ADPCM sound
    u8 param_fifo_empty : 1;         // triggered before writing 1st byte
    u8 param_fifo_write_ready : 1;   // triggered after writing 16 bytes
    u8 response_fifo_not_empty : 1;  // triggered after reading LAST byte
    u8 data_fifo_not_empty : 1;      // triggered after reading LAST byte
    u8 transmit_busy : 1;            // Command/parameter transmission busy
  };

  CdromStatusRegister() {
    param_fifo_empty = true;
    param_fifo_write_ready = true;
  }
};

union CdromMode {
  u8 byte{};

  struct {
    u8 cd_da_read : 1;    // (0=Off, 1=Allow to Read CD-DA Sectors; ignore missing EDC)
    u8 auto_pause : 1;    // (0=Off, 1=Auto Pause upon End of Track) ;for Audio Play
    u8 report : 1;        // (0=Off, 1=Enable Report-Interrupts for Audio Play)
    u8 xa_filter : 1;     // (0=Off, 1=Process only XA-ADPCM sectors that match Setfilter)
    u8 ignore_bit : 1;    // (0=Normal, 1=Ignore Sector Size and Setloc position)
    u8 _sector_size : 1;  // (0=800h=DataOnly, 1=924h=WholeSectorExceptSyncBytes)
    u8 xa_adpcm : 1;      // (0=Off, 1=Send XA-ADPCM sectors to SPU Audio Input)
    u8 speed : 1;         // (0=Normal speed, 1=Double speed)
  };

  void reset() { byte = 0; }
  u32 sector_size() const { return _sector_size ? 0x924 : 0x800; }
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

class CdromDrive {
 public:
  void init(cpu::Interrupts* interrupts);
  void insert_disk_file(const fs::path& file_path);
  void step();
  u8 read_reg(address addr_rebased);
  void write_reg(address addr_rebased, u8 val);
  u8 read_byte();
  u32 read_word();

 private:
  void execute_command(u8 cmd);
  void push_response(CdromResponseType type, std::initializer_list<u8> bytes);
  void push_response(CdromResponseType type, u8 byte);
  void push_response_stat(CdromResponseType type);
  void command_error();
  u8 get_param();
  bool is_data_buf_empty();

  static const char* get_reg_name(u8 reg, u8 index, bool is_read);
  static const char* get_cmd_name(u8 cmd);

 private:
  CdromDisk m_disk;

  CdromStatusRegister m_reg_status{};
  CdromStatusCode m_stat_code{};
  CdromMode m_mode{};
  CdromReadState m_read_state{};

  u32 m_seek_sector{};
  u32 m_read_sector{};

  std::deque<u8> m_param_fifo{};
  std::deque<CdromResponseType> m_irq_fifo{};
  std::deque<u8> m_resp_fifo{};

  u8 m_reg_int_enable{};
  u32 m_steps_until_read_sect{ READ_SECTOR_DELAY_STEPS };

  buffer m_read_buf{};
  buffer m_data_buf{};
  u32 m_data_buffer_index{};

  bool m_muted{ false };

  cpu::Interrupts* m_interrupts{};
};

}  // namespace io
