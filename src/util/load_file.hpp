#pragma once

#include <util/fs.hpp>
#include <util/types.hpp>

#include <vector>

namespace util {

buffer load_file(fs::path const& filepath);

}  // namespace util
