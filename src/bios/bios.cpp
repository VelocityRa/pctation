#include <bios/bios.hpp>
#include <util/fs.hpp>
#include <util/load_file.hpp>

#include <memory>

namespace bios {

Bios::Bios(fs::path const& path) {
  auto buf = util::load_file(path);

  m_data = std::make_unique<std::array<byte, memory::BIOS_SIZE>>();

  std::copy(buf.begin(), buf.end(), m_data->begin());
}

u32 Bios::read32(u32 addr) const {
  return *(u32*)(m_data.get()->data() + addr);
}

}  // namespace bios
