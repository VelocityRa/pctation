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

inline u32 leading_zeroes(s32 n) {
  u32 zeroes{};
  if ((n & 0x80000000) == 0)
    n = ~n;

  while ((n & 0x80000000) != 0) {
    zeroes++;
    n <<= 1;
  }
  return zeroes;
}

}  // namespace bit_utils
