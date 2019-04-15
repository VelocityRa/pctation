#pragma once

#include <util/types.hpp>

#include <algorithm>
#include <array>
#include <memory>

namespace memory {

template <size_t MemorySize>
class Addressable {
 public:
  Addressable() {
    m_data = std::make_unique<std::array<byte, MemorySize>>();
    std::fill(m_data->begin(), m_data->end(), 0xDE);
  }

  template <typename ValueType>
  ValueType read(address addr) const {
    return *(ValueType*)(m_data.get()->data() + addr);
  }

  template <typename ValueType>
  void write(address addr, ValueType val) {
    *(ValueType*)(m_data.get()->data() + addr) = val;
  }

 protected:
  std::unique_ptr<std::array<byte, MemorySize>> m_data;
};

}  // namespace memory
