#include <renderer/screen_renderer.hpp>

#include <renderer/shader.hpp>

#include <glbinding/gl/gl.h>

#include <stdexcept>

using namespace gl;

namespace renderer {

const GLuint ATTRIB_INDEX_POSITION = 0;
const GLuint ATTRIB_INDEX_TEXCOORD = 1;

ScreenRenderer::ScreenRenderer() {
  // Load and compile shaders
  m_shader_program_screen = renderer::load_shaders("screen");

  if (!m_shader_program_screen)
    throw std::runtime_error("Couldn't compile screen shader");

  glUseProgram(m_shader_program_screen);

  // Generate VAO
  glGenVertexArrays(1, &m_vao);
  glBindVertexArray(m_vao);

  // Generate and configure VBO
  glGenBuffers(1, &m_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

  const auto x = 1.f;
  const auto y = 1.f;

  const float vertices[] = {
    // Position Texcoords
    -1.f, 1.f, 0.0f, 0.0f,  // Top-left
    -1.f, -y,  0.0f, 1.0f,  // Bottom-left
    x,    1.f, 1.0f, 0.0f,  // Top-right
    x,    -y,  1.0f, 1.0f,  // Bottom-right
  };

  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  const auto vertex_stride = 4 * sizeof(float);
  const auto position_offset = 0 * sizeof(float);
  const auto texcoord_offset = 2 * sizeof(float);

  glEnableVertexAttribArray(ATTRIB_INDEX_POSITION);
  glVertexAttribPointer(ATTRIB_INDEX_POSITION, 2, GL_FLOAT, GL_FALSE, vertex_stride,
                        (const void*)position_offset);

  glEnableVertexAttribArray(ATTRIB_INDEX_TEXCOORD);
  glVertexAttribPointer(ATTRIB_INDEX_TEXCOORD, 2, GL_FLOAT, GL_FALSE, vertex_stride,
                        (const void*)texcoord_offset);

  // Generate and configure screen texture
  glGenTextures(1, &m_tex_screen);
  bind_screen_texture();

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // Get uniforms
  m_u_tex_size = glGetUniformLocation(m_shader_program_screen, "u_tex_size");

  glBindVertexArray(0);
}

void ScreenRenderer::render(const void* vram_data) const {
  // Bind needed state
  glBindVertexArray(m_vao);
  glUseProgram(m_shader_program_screen);
  bind_screen_texture();

  // Upload screen texture
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 1024);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_screen_width, m_screen_height, GL_RGBA,
                  GL_UNSIGNED_SHORT_1_5_5_5_REV, vram_data);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

  // Set uniforms
  glUniform2f(m_u_tex_size, (f32)m_screen_width, (f32)m_screen_height);

  // Draw screen
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void ScreenRenderer::set_texture_size(s32 width, s32 height) {
  bind_screen_texture();

  // If screen texture dimensions changed
  if (width != m_screen_width || height != m_screen_height) {
    // Configure texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV,
                 nullptr);
    m_screen_width = width;
    m_screen_height = height;
  }
}

void ScreenRenderer::bind_screen_texture() const {
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_tex_screen);
}

ScreenRenderer::~ScreenRenderer() {
  glDeleteTextures(1, &m_tex_screen);
  glDeleteBuffers(1, &m_vbo);
  glDeleteVertexArrays(1, &m_vao);
  glDeleteProgram(m_shader_program_screen);
}

}  // namespace renderer
