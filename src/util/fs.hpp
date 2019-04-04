#pragma once

#ifdef _MSC_VER
#include <filesystem>
#else
#include <experimental/filesystem>
#endif

namespace fs = std::experimental::filesystem;
