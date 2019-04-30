#pragma once

#include <memory/addressable.hpp>
#include <memory/map.hpp>
#include <util/fs.hpp>
#include <util/types.hpp>

#include <array>
#include <memory>

namespace memory {

class Expansion : public memory::Addressable<memory::EXPANSION_1_SIZE> {
 public:
  explicit Expansion(fs::path const& path);
};

}  // namespace memory
