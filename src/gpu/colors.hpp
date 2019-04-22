#pragma once

#include <util/types.hpp>

namespace gpu {

union RGB32 {
  struct {
    u8 r;
    u8 g;
    u8 b;
    u8 _pad;
  };
  u32 word{};

  static RGB32 fromRGBword(u32 word) {
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
  u16 word{};

  static RGB16 fromRGB(u8 r, u8 g, u8 b) {
    RGB16 c16{};
    c16.r = r >> 3;
    c16.g = g >> 3;
    c16.b = b >> 3;
    c16.mask = 0;
    return c16;
  }
  static RGB16 fromRGB32(RGB32 c32) { return fromRGB(c32.r, c32.g, c32.b); }
};

}  // namespace gpu
