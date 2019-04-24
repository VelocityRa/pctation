#include <memory/ram.hpp>

#include <util/load_file.hpp>
#include <util/log.hpp>

#include <gsl-lite.hpp>

#include <algorithm>
#include <array>

namespace memory {

Ram::Ram(fs::path psxexe_path) : m_psxexe_path(psxexe_path) {
  std::fill(m_data->begin(), m_data->end(), 0);
}

bool Ram::load_executable(PSEXELoadInfo& out_psx_load_info) const {
  if (m_psxexe_path.empty())
    return false;

  const buffer& psx_exe_buf = util::load_file(m_psxexe_path);

  if (psx_exe_buf.empty())
    return false;

  struct PSXEXEHeader {
    char magic[8];  // "PS-X EXE"
    u8 pad0[8];
    u32 pc;         // initial PC
    u32 r28;        // initial R28
    u32 load_addr;  // destination address in RAM
    u32 filesize;   // excluding header & must be N*0x800
    u32 unk0[2];
    u32 memfill_start;
    u32 memfill_size;
    u32 r29_r30;         // initial r29 and r30 base
    u32 r29_r30_offset;  // initial r29 and r30 offset, added to above
    // etc, we don't care about anything else
  };

  const auto psx_exe = (PSXEXEHeader*)psx_exe_buf.data();

  if (std::strcmp(&psx_exe->magic[0], "PS-X EXE")) {
    LOG_ERROR("Not a valid PS-X EXE file!");
    return false;
  }

  // We don't support memfill
  Ensures(psx_exe->memfill_start == 0);
  Ensures(psx_exe->memfill_size == 0);

  out_psx_load_info.pc = psx_exe->pc;
  out_psx_load_info.r28 = psx_exe->r28;
  out_psx_load_info.r29_r30 = psx_exe->r29_r30 + psx_exe->r29_r30_offset;

  constexpr auto PSXEXE_HEADER_SIZE = 0x800;

  const auto copy_src_begin = psx_exe_buf.data() + PSXEXE_HEADER_SIZE;
  const auto copy_src_end = copy_src_begin + psx_exe->filesize;
  const auto copy_dest_begin = m_data->data() + (psx_exe->load_addr & 0x7FFFFFFF);

  std::copy(copy_src_begin, copy_src_end, copy_dest_begin);

  return true;
}

Scratchpad::Scratchpad() {
  std::fill(m_data->begin(), m_data->end(), 0);
}

}  // namespace memory
