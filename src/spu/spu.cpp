#include <spu/spu.hpp>

#include <util/types.hpp>

#include <algorithm>

namespace spu {
Spu::Spu() {
  std::fill(m_data->begin(), m_data->end(), 0);

  write<u16>(0x1AA, 0x8000);  // hard-code SPUCNT
  //  write<u16>(0x1AE, 0x10);  // hard-code SPUCNT
}
}  // namespace spu
