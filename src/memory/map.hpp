#pragma once

#include <bios/bios.hpp>
#include <memory/range.hpp>

namespace memory {
namespace map {

// Memory map
static constexpr Range BIOS{ 0xBFC00000, bios::BIOS_SIZE };
static constexpr Range MEM_CONTROL1{ 0x1F801000, 0x24 };
static constexpr Range MEM_CONTROL2{ 0x1F801060, 0x4 };
static constexpr Range MEM_CONTROL3{ 0xFFFE0130, 0x4 };

}  // namespace map
}  // namespace memory
