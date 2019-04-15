#pragma once

#include <memory/addressable.hpp>
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

class Ram : public Addressable<memory::RAM_SIZE> {
 public:
  explicit Ram(fs::path psxexe_path);
  bool load_executable(PSEXELoadInfo& out_psx_load_info) const;  // Returns true on successful load
  const std::array<byte, RAM_SIZE>& data() const { return *m_data; }

 private:
  fs::path m_psxexe_path;
};

class Scratchpad : public Addressable<memory::SCRATCHPAD_SIZE> {};

}  // namespace memory
