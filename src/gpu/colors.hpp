#pragma once

#include <util/types.hpp>

#include <algorithm>

namespace gpu {

union RGB32 {
  struct {
    u8 r;
    u8 g;
    u8 b;
    u8 _pad;
  };
  u32 word{};

  static RGB32 from_word(u32 word) {
    RGB32 rgb32;
    rgb32.word = word;
    return rgb32;
  }
};

union RGB16 {
  struct {
    u16 r : 5;
    u16 g : 5;
    u16 b : 5;
    u16 mask : 1;
  };
  RGB16 operator*(const RGB32 rhs) {
    r = std::min<u16>(r * rhs.r, 31);
    g = std::min<u16>(g * rhs.g, 31);
    b = std::min<u16>(b * rhs.b, 31);
    return *this;
  }
  RGB16 operator*=(const RGB32 rhs) { return *this * rhs; }
  RGB16 operator*(float n) {
    r = std::min<u16>(r * (u8)n, 31);
    g = std::min<u16>(g * (u8)n, 31);
    b = std::min<u16>(b * (u8)n, 31);
    return *this;
  }
  RGB16 operator*=(float rhs) { return *this * rhs; }
  u16 word{};

  static RGB16 from_RGB(u8 r, u8 g, u8 b) {
    RGB16 c16{};
    c16.r = r >> 3;
    c16.g = g >> 3;
    c16.b = b >> 3;
    c16.mask = 0;
    return c16;
  }
  static RGB16 from_RGB32(RGB32 c32) { return from_RGB(c32.r, c32.g, c32.b); }
  static RGB16 from_word(u16 word) {
    RGB16 rgb16;
    rgb16.word = word;
    return rgb16;
  }
};

}  // namespace gpu
