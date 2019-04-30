#include <memory/expansion.hpp>

#include <gsl-lite.hpp>
#include <util/fs.hpp>
#include <util/load_file.hpp>

#include <algorithm>
#include <array>
#include <memory>

namespace memory {

Expansion::Expansion(fs::path const& bootstrap_path) {
  // Init Expansion memory with 0xFF
  m_data = std::make_unique<std::array<byte, memory::EXPANSION_1_SIZE>>();
  std::fill(m_data->begin(), m_data->end(), 0xFF);

  // Load bootstrap
  if (!bootstrap_path.empty()) {
    auto buf = util::load_file(bootstrap_path);
    Ensures(buf.size() <= memory::EXPANSION_1_SIZE);
    std::copy(buf.begin(), buf.end(), m_data->begin());
  }

  // Set cheat (Action Replay) switch to ON
  (*m_data.get())[0x20018] = 1;
}

}  // namespace memory
