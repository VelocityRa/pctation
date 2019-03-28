#pragma once

#include <memory/range.hpp>

namespace memory {

// Memory sizes
static constexpr u32 BIOS_SIZE = 512 * 1024;      // All BIOS images are 512KB
static constexpr u32 RAM_SIZE = 2 * 1024 * 1024;  // Main RAM, 2 MB

namespace map {

// Memory map (physical addresses)
static constexpr Range RAM{ 0x00000000, RAM_SIZE };
static constexpr Range BIOS{ 0x1FC00000, BIOS_SIZE };
static constexpr Range MEM_CONTROL1{ 0x1F801000, 0x24 };
static constexpr Range MEM_CONTROL2{ 0x1F801060, 0x4 };
static constexpr Range MEM_CONTROL3{ 0xFFFE0130, 0x4 };
static constexpr Range SPU{ 0x1F801C00, 0x280 };
static constexpr Range EXPANSION_1{ 0x1F000000, 512 * 1024 };
static constexpr Range EXPANSION_2{ 0x1F802000, 0x42 };
static constexpr Range IRQ_CONTROL{ 0x1F801070, 8 };

}  // namespace map

inline u32 mask_region(u32 addr) {
  constexpr u32 region_mask[8] = {
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,  // KUSEG: 2048MB, already physaddr, no need to mask
    0x7FFFFFFF,                                      // KSEG0: 512MB, mask top bit
    0x1FFFFFFF,                                      // KSEG1: 512MB, mask top 3 bits
    0xFFFFFFFF, 0xFFFFFFFF                           // KSEG1: 1024MB, already physaddr, no need to mask
  };
  // Use addr's top 3 bits to determine the region and index into region_map
  return addr & region_mask[addr >> 29];
}

}  // namespace memory
