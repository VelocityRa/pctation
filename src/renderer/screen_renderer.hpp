#pragma once

#include <glbinding/gl/types.h>

#include <util/types.hpp>

namespace renderer {

class ScreenRenderer {
 public:
  explicit ScreenRenderer();
  ~ScreenRenderer();

  void render(const void* vram_data) const;
  void bind_screen_texture() const;
  void set_texture_size(s32 width, s32 height);

 private:
  s32 m_screen_width{};
  s32 m_screen_height{};

  // Shaders
  gl::GLuint m_shader_program_screen{};

  // Other OpenGL objects
  gl::GLuint m_vao{};
  gl::GLuint m_vbo{};
  gl::GLuint m_tex_screen{};
  gl::GLuint m_u_tex_size{};
};

}  // namespace renderer