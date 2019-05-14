#pragma once

#include <util/types.hpp>

namespace bit_utils {

template <u32 sign_bit, typename T>
T sign_extend(u64 n) {
  static_assert(sign_bit > 0 && sign_bit < 63, "sign bit out of range");

  T mask = (1LL << sign_bit) - 1;
  const auto sign = (n & (1LL << sign_bit)) != 0;

  T val = n & mask;

  if (sign)
    val |= ~mask;

  return val;
}

}  // namespace bit_utils
