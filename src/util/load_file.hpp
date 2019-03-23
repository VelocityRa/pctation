#pragma once

#include <fs.hpp>
#include <types.hpp>

#include <vector>

namespace util {

buffer load_file(fs::path const& filepath);

}  // namespace util
