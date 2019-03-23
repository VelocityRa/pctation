#include <fs.hpp>
#include <load_file.hpp>

#include <cstddef>
#include <fstream>
#include <vector>

namespace util {

buffer load_file(fs::path const& filepath) {
  std::ifstream ifs(filepath, std::ios::binary | std::ios::ate);

  if (!ifs)
    throw std::runtime_error(filepath.string() + ": " + std::strerror(errno));

  const auto end = ifs.tellg();
  ifs.seekg(0, std::ios::beg);

  const auto size = std::size_t(end - ifs.tellg());

  if (size == 0)  // avoid undefined behavior
    return {};

  const buffer buffer(size);

  if (!ifs.read((char*)buffer.data(), buffer.size()))
    throw std::runtime_error(filepath.string() + ": " + std::strerror(errno));

  return buffer;
}

}  // namespace util
