#pragma once

#include <memory/map.hpp>
#include <util/fs.hpp>
#include <util/types.hpp>

#include <array>
#include <memory>

namespace memory {

struct PSEXELoadInfo {
  u32 pc;
  u32 r28;
  u32 r29_r30;
};

class Ram {
 public:
  explicit Ram(fs::path psxexe_path);
  u32 read32(u32 addr) const;
  u16 read16(u32 addr);
  u8 read8(u32 addr) const;
  void write32(u32 addr, u32 val);
  void write16(u32 addr, u16 val);
  void write8(u32 addr, u8 val);
  bool load_executable(PSEXELoadInfo& out_psx_load_info);  // Returns true on successful load
  const std::array<byte, RAM_SIZE>& data() const { return *m_data; }

  // Scratchpad memory
  // TODO: refactor
  u32 read32_scratch(u32 addr) const;
  u16 read16_scratch(u32 addr);
  u8 read8_scratch(u32 addr) const;
  void write32_scratch(u32 addr, u32 val);
  void write16_scratch(u32 addr, u16 val);
  void write8_scratch(u32 addr, u8 val);

 private:
  std::unique_ptr<std::array<byte, RAM_SIZE>> m_data;
  std::unique_ptr<std::array<byte, SCRATCHPAD_SIZE>> m_data_scratch;
  fs::path m_psxexe_path;
};

}  // namespace memory
