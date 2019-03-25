#pragma once

#include <memory/range.hpp>

namespace memory {

// Memory sizes
static constexpr u32 BIOS_SIZE = 512 * 1024;      // All BIOS images are 512KB
static constexpr u32 RAM_SIZE = 2 * 1024 * 1024;  // 2 MB

namespace map {

// Memory map
static constexpr Range RAM{ 0xA0000000, RAM_SIZE };
static constexpr Range BIOS{ 0xBFC00000, BIOS_SIZE };
static constexpr Range MEM_CONTROL1{ 0x1F801000, 0x24 };
static constexpr Range MEM_CONTROL2{ 0x1F801060, 0x4 };
static constexpr Range MEM_CONTROL3{ 0xFFFE0130, 0x4 };

}  // namespace map
}  // namespace memory
