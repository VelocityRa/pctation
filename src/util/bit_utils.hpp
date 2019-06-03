#pragma once

#include <util/types.hpp>

namespace bit_utils {

template <u32 sign_bit, typename T>
constexpr T sign_extend(u64 n) {
  static_assert(sign_bit > 0 && sign_bit < 63, "sign bit out of range");

  T mask = (1LL << sign_bit) - 1;
  const auto sign = (n & (1LL << sign_bit)) != 0;

  T val = n & mask;

  if (sign)
    val |= ~mask;

  return val;
}

template <typename T>
constexpr std::size_t bit_size() noexcept {
  return sizeof(T) * CHAR_BIT;
}

template <typename T>
constexpr size_t leading_zeroes(T n) {
  using UnsignedT = typename std::make_unsigned<T>::type;

  if (n == 0)
    return 0;

  constexpr auto top_bit_shift = bit_size<T>() - 1;
  constexpr u64 top_bit = 1 << top_bit_shift;

  size_t zeroes{};

  while (!(static_cast<UnsignedT>(n) & top_bit)) {
    zeroes++;
    n <<= 1;
  }
  return zeroes;
}

}  // namespace bit_utils
