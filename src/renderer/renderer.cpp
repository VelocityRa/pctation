#include <renderer/renderer.hpp>

#include <renderer/shader.hpp>

#include <glbinding/gl/gl.h>

#include <string>

using namespace gl;

namespace renderer {

const GLuint ATTRIB_INDEX_POSITION = 0;
const GLuint ATTRIB_INDEX_COLOR = 1;

Renderer::Renderer() {
  // Load and compile shaders

  m_shader_program_gouraud = renderer::load_shaders("gouraud");

  if (!m_shader_program_gouraud)
    throw std::exception("Couldn't compile gouraud shader");

  glUseProgram(m_shader_program_gouraud);

  // Generate VAO
  glGenVertexArrays(1, &m_vao);
  glBindVertexArray(m_vao);

  // Initialize vertex attributes for each buffer

  m_vb_positions.init();
  glEnableVertexAttribArray(ATTRIB_INDEX_POSITION);
  glVertexAttribIPointer(ATTRIB_INDEX_POSITION, 2, GLenum::GL_SHORT, 0, nullptr);

  m_vb_colors.init();
  glEnableVertexAttribArray(ATTRIB_INDEX_COLOR);
  glVertexAttribIPointer(ATTRIB_INDEX_COLOR, 3, GLenum::GL_UNSIGNED_BYTE, 0, nullptr);

  glBindVertexArray(0);
}

Renderer::~Renderer() {
  glDeleteVertexArrays(1, &m_vao);
  glDeleteProgram(m_shader_program_gouraud);
}

void Renderer::push_triangle(Position3 positions, Color3 colors) {
  if (m_vertex_count + 3 > VERTEX_BUFFER_LEN) {
    // Not enough room in the buffer, draw what we have
    draw();
  }

  for (auto i = 0; i < 3; ++i) {
    m_vb_positions.set(m_vertex_count, positions[i]);
    m_vb_colors.set(m_vertex_count, colors[i]);

    m_vertex_count++;
  }
}

void Renderer::push_quad(Position4 positions, Color4 colors) {
  if (m_vertex_count + 6 > VERTEX_BUFFER_LEN) {
    // Not enough room in the buffer, draw what we have
    draw();
  }

  for (auto i = 0; i < 3; ++i) {
    m_vb_positions.set(m_vertex_count, positions[i]);
    m_vb_colors.set(m_vertex_count, colors[i]);

    m_vertex_count++;
  }
  for (auto i = 1; i < 4; ++i) {
    m_vb_positions.set(m_vertex_count, positions[i]);
    m_vb_colors.set(m_vertex_count, colors[i]);

    m_vertex_count++;
  }
}

void Renderer::draw() {
  // Make sure the data in the buffers are flushed to the GPU
  glMemoryBarrier(MemoryBarrierMask::GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);

  glUseProgram(m_shader_program_gouraud);
  glBindVertexArray(m_vao);

  // Issue draw call
  glDrawArrays(GLenum::GL_TRIANGLES, 0, m_vertex_count);

  // Wait for GPU to complete draw
  const auto sync_gpu_complete =
      glFenceSync(GLenum::GL_SYNC_GPU_COMMANDS_COMPLETE, UnusedMask::GL_UNUSED_BIT);
  while (true) {
    const auto r =
        glClientWaitSync(sync_gpu_complete, SyncObjectMask::GL_SYNC_FLUSH_COMMANDS_BIT, 10'000'000);
    if (r == GLenum::GL_ALREADY_SIGNALED || r == GLenum::GL_CONDITION_SATISFIED)
      break;
  }

  // Reset vertex count which is the index we write to, effectively resetting the buffers
  m_vertex_count = 0;
}

}  // namespace renderer
