#include <memory/range.hpp>

namespace memory {

bool Range::contains(address addr, address& out_offset) const {
  const auto does_contain = m_start <= addr && addr < m_start + m_size;

  if (does_contain)
    out_offset = addr - m_start;

  return does_contain;
}

}  // namespace memory
