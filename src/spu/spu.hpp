#pragma once

#include <memory/addressable.hpp>
#include <memory/map.hpp>

namespace spu {

class Spu : public memory::Addressable<memory::SPU_SIZE> {
 public:
  Spu();
  // TODO
 private:
};

}  // namespace spu
