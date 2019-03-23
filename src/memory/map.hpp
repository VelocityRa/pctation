#pragma once

#include <bios/bios.hpp>
#include <memory/range.hpp>

namespace memory {
namespace map {

// Memory map
static constexpr Range BIOS{ 0xBFC00000, bios::BIOS_SIZE };

}  // namespace map
}  // namespace memory
