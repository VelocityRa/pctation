#pragma once

#include <memory/addressable.hpp>
#include <memory/map.hpp>
#include <util/fs.hpp>
#include <util/types.hpp>

#include <array>
#include <memory>

namespace bios {

class Bios : public memory::Addressable<memory::BIOS_SIZE> {
 public:
  explicit Bios(fs::path const& path);
};

}  // namespace bios
