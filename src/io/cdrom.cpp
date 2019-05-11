#include <io/cdrom.hpp>

#include <cpu/interrupt.hpp>
#include <util/log.hpp>

#include <gsl-lite.hpp>

namespace io {

void Cdrom::init(cpu::Interrupts* interrupts) {
  m_interrupts = interrupts;
}

void Cdrom::step() {
  m_reg_status.transmit_busy = false;

  if (!m_irq_fifo.empty()) {
    auto irq_triggered = m_irq_fifo.front() & 0b111;
    auto irq_mask = m_reg_int_enable & 0b111;

    if (irq_triggered & irq_mask)
      m_interrupts->trigger(cpu::IrqType::CDROM);
  }

  if (m_status_code.playing) {
    LOG_ERROR("Playing CD audio unsupported");
    return;
  }

  if (m_status_code.reading) {
    // TODO
  }
}

u8 Cdrom::read_reg(address addr_rebased) {
  const u8 reg = addr_rebased;
  const u8 reg_index = m_reg_status.index;

  u8 val = 0;

  if (reg == 0) {  // Status Register
    val = m_reg_status.byte;
  } else if (reg == 1 && reg_index == 0) {  // Command Register
  } else if (reg == 1) {                    // Response FIFO
    if (!m_resp_fifo.empty()) {
      val = m_resp_fifo.front();
      m_resp_fifo.pop_front();

      if (m_resp_fifo.empty())
        m_reg_status.response_fifo_not_empty = false;
    }
  } else if (reg == 2) {                                        // Data FIFO
  } else if (reg == 3 && (reg_index == 0 || reg_index == 2)) {  // Interrupt Enable Register
    val = m_reg_int_enable;
  } else if (reg == 3 && (reg_index == 1 || reg_index == 3)) {  // Interrupt Flag Register
    val = 0b11100000;                                           // these bits are always set

    if (!m_irq_fifo.empty())
      val |= m_irq_fifo.front() & 0b111;
  } else {
    LOG_CRITICAL("Unknown combination, CDREG{}.{}", reg, reg_index);
  }

  auto reg_name = get_reg_name(reg, reg_index, true);
  LOG_ERROR("CDROM read {} (CDREG{}.{}) val: 0x{:02X} ({:#010b})", reg_name, reg, reg_index, val, val);

  return val;
}

void Cdrom::write_reg(address addr_rebased, u8 val) {
  const u8 reg = addr_rebased;
  const u8 reg_index = m_reg_status.index;

  if (reg == 0) {  // Index Register
    m_reg_status.index = val & 0b11;
    return;                                 // Don't log in this case
  } else if (reg == 1 && reg_index == 0) {  // Command Register
    execute_command(val);
  } else if (reg == 1 && reg_index == 1) {  // Sound Map Data Out
  } else if (reg == 1 && reg_index == 2) {  // Sound Map Coding Info
  } else if (reg == 1 && reg_index == 3) {  // Audio Volume for Right-CD-Out to Right-SPU-Input
  } else if (reg == 2 && reg_index == 0) {  // Parameter FIFO
    Ensures(m_param_fifo.size() < MAX_FIFO_SIZE);

    m_param_fifo.push_back(val);
    m_reg_status.param_fifo_empty = false;
    m_reg_status.param_fifo_write_ready = (m_param_fifo.size() < MAX_FIFO_SIZE);
  } else if (reg == 2 && reg_index == 1) {  // Interrupt Enable Register
    m_reg_int_enable = val;
  } else if (reg == 2 && reg_index == 2) {  // Audio Volume for Left-CD-Out to Left-SPU-Input
  } else if (reg == 2 && reg_index == 3) {  // Audio Volume for Right-CD-Out to Left-SPU-Input
  } else if (reg == 3 && reg_index == 0) {  // Request Register
  } else if (reg == 3 && reg_index == 1) {  // Interrupt Flag Register
    if (val & 0x40) {                       // Reset Parameter FIFO
      m_param_fifo.clear();
      m_reg_status.param_fifo_empty = true;
      m_reg_status.param_fifo_write_ready = true;
    }
    if (!m_irq_fifo.empty())
      m_irq_fifo.pop_front();
  } else if (reg == 3 && reg_index == 2) {  // Audio Volume for Left-CD-Out to Right-SPU-Input
  } else if (reg == 3 && reg_index == 3) {  // Audio Volume Apply Changes
  } else {
    LOG_CRITICAL("Unknown combination, CDREG{}.{} val: {:02X}", reg, reg_index, val);
  }

  auto reg_name = get_reg_name(reg, reg_index, false);
  LOG_ERROR("CDROM write {} (CDREG{}.{}) val: 0x{:02X} ({:#010b})", reg_name, reg, reg_index, val, val);
}

void Cdrom::execute_command(u8 cmd) {
  m_irq_fifo.clear();
  m_resp_fifo.clear();

  LOG_CRITICAL("CDROM command issued: {} ({:02X})", get_cmd_name(cmd), cmd);

  if (!m_param_fifo.empty())
    LOG_CRITICAL("Parameters: [{:02X}]", fmt::join(m_param_fifo, ", "));

  switch (cmd) {
    case 0x01: {  // Getstat
      push_response(FirstInt3, m_status_code.byte);
      break;
    }
    case 0x02: {  // Setloc
      push_response(FirstInt3, m_status_code.byte);
      m_seek_loc = 0;  // TODO: bcd crap
      break;
    }
    case 0x06: {  // ReadN
      push_response(FirstInt3, m_status_code.byte);

      m_status_code.set_state(CdromReadState::Reading);

      push_response(SecondInt1, m_status_code.byte);

      break;
    }
    case 0x0E: {  // Setmode
      push_response(FirstInt3, m_status_code.byte);
      // TODO: bit 4 behaviour
      Expects(!(get_param() & 0b10000));
      m_mode.byte = get_param();
      break;
    }
    case 0x15: {  // SeekL
      push_response(FirstInt3, m_status_code.byte);

      m_read_loc = m_seek_loc;
      m_status_code.set_state(CdromReadState::Seeking);

      push_response(SecondInt2, m_status_code.byte);

      break;
    }
    case 0x19: {  // Test
      const auto subfunction = get_param();
      LOG_CRITICAL("  CDROM command subfuncion: {:02X}", subfunction);

      switch (subfunction) {
        case 0x20:  // Get CDROM BIOS date/version (yy,mm,dd,ver)
          push_response(
              FirstInt3,
              { 0x94, 0x09, 0x19, 0xC0 });  // Send reponse of PSX (PU-7), 18 Nov 1994, version vC0 (b)
          break;
        default: {
          command_error();
          LOG_CRITICAL("Unhandled Test subfunction {:02X}", subfunction);
          break;
        }
      }
      break;
    }
    case 0x1A: {  // GetID
      // No Disk
      push_response(FirstInt3, m_status_code.byte);
      push_response(SecondInt2, { 0x08, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
      break;
    }
    case 0x0A: {  // Init
      push_response(FirstInt3, m_status_code.byte);

      m_status_code.reset();
      m_status_code.spindle_motor_on = true;

      m_mode.reset();

      push_response(SecondInt2, m_status_code.byte);
    }
    default: {
      command_error();
      LOG_CRITICAL("Unhandled CDROM command {:02X}", cmd);
      break;
    }
  }

  if (!m_resp_fifo.empty())
    LOG_CRITICAL("Response: [{:02X}]", fmt::join(m_resp_fifo, ", "));

  m_param_fifo.clear();

  m_reg_status.transmit_busy = true;
  m_reg_status.param_fifo_empty = true;
  m_reg_status.param_fifo_write_ready = true;
  m_reg_status.adpcm_fifo_empty = false;
}

void Cdrom::command_error() {
  push_response(ErrorInt5, { 0x11, 0x40 });
}

u8 Cdrom::get_param() {
  return m_param_fifo.front();
}

void Cdrom::push_response(CdromResponseType type, std::initializer_list<u8> bytes) {
  // First we write the type (INT value) in the Interrupt FIFO
  m_irq_fifo.push_back(type);

  // Then we write the response's data (args) to the Response FIFO
  for (auto response_byte : bytes) {
    if (m_resp_fifo.size() < MAX_FIFO_SIZE) {
      m_resp_fifo.push_back(response_byte);
      m_reg_status.response_fifo_not_empty = true;
    } else
      LOG_WARN("CDROM response 0x{:02} lost, FIFO was full", response_byte);
  }
}

void Cdrom::push_response(CdromResponseType type, u8 byte) {
  push_response(type, { byte });
}

const char* Cdrom::get_cmd_name(u8 cmd) {
  const char* cmd_names[] = { "Sync",       "Getstat",   "Setloc",  "Play",     "Forward", "Backward",
                              "ReadN",      "MotorOn",   "Stop",    "Pause",    "Init",    "Mute",
                              "Demute",     "Setfilter", "Setmode", "Getparam", "GetlocL", "GetlocP",
                              "SetSession", "GetTN",     "GetTD",   "SeekL",    "SeekP",   "-",
                              "-",          "Test",      "GetID",   "ReadS",    "Reset",   "GetQ",
                              "ReadTOC",    "VideoCD" };

  if (cmd <= 0x1F)
    return cmd_names[cmd];
  if (0x50 <= cmd && cmd <= 0x57)
    return "Secret";
  return "<unknown>";
}

const char* Cdrom::get_reg_name(u8 reg, u8 index, bool is_read) {
  // clang-format off
  if (is_read) {
    if (reg == 0) return "Status Register";
    if (reg == 1 && index == 0) return "Command Register";
    if (reg == 1) return "Response FIFO";
    if (reg == 2) return "Data FIFO";
    if (reg == 3 && (index == 0 || index == 2)) return "Interrupt Enable Register";
    if (reg == 3 && (index == 1 || index == 3)) return "Interrupt Flag Register";
  } else {
    if (reg == 0) return "Index Register";
    if (reg == 1 && index == 0) return "Command Register";
    if (reg == 1 && index == 1) return "Sound Map Data Out";
    if (reg == 1 && index == 2) return "Sound Map Coding Info";
    if (reg == 1 && index == 3) return "Audio Volume for Right-CD-Out to Right-SPU-Input";
    if (reg == 2 && index == 0) return "Parameter FIFO";
    if (reg == 2 && index == 1) return "Interrupt Enable Register";
    if (reg == 2 && index == 2) return "Audio Volume for Left-CD-Out to Left-SPU-Input";
    if (reg == 2 && index == 3) return "Audio Volume for Right-CD-Out to Left-SPU-Input";
    if (reg == 3 && index == 0) return "Request Register";
    if (reg == 3 && index == 1) return "Interrupt Flag Register";
    if (reg == 3 && index == 2) return "Audio Volume for Left-CD-Out to Right-SPU-Input";
    if (reg == 3 && index == 3) return "Audio Volume Apply Changes";
  }
  // clang-format on
  return "<unknown>";
}

}  // namespace io
