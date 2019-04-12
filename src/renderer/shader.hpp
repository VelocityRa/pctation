#pragma once

#include <glbinding/gl/types.h>

#include <string>

namespace renderer {

gl::GLuint load_shaders(const std::string& shader_name);

}
