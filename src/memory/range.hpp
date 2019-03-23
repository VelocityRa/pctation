#pragma once

#include <util/types.hpp>

namespace memory {

class Range {
 public:
  constexpr Range(address start, u32 size) : m_start(start), m_size(size) {}

  bool contains(address addr, address& out_addr_rebased) const;

 private:
  u32 m_start;
  u32 m_size;
};

}  // namespace memory
