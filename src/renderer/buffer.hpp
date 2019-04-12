#pragma once

#include <util/types.hpp>

#include <glbinding/gl/gl.h>

#include <cstring>
#include <exception>

using namespace gl;

namespace renderer {

template <typename T, u32 Size>
class PersistentMappedBuffer {
  static constexpr u32 element_size = sizeof T;
  static constexpr u32 buffer_size = element_size * Size;

 public:
  void init() {
    // Generate and bind a buffer
    glGenBuffers(1, &m_object);
    bind();

    const auto access = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT;

    // Allocate buffer memory
    glBufferStorage(GLenum::GL_ARRAY_BUFFER, buffer_size, nullptr, access);

    // Map the buffer
    m_memory = static_cast<T*>(glMapBufferRange(GLenum::GL_ARRAY_BUFFER, 0, buffer_size, access));

    // Reset it to 0
    std::memset(m_memory, 0, buffer_size);
  }

  void set(u32 index, T val) {
    if (index > Size)
      throw std::runtime_error("gl buffer overflow");

    m_memory[index] = val;
  }

  void bind() { glBindBuffer(GLenum::GL_ARRAY_BUFFER, m_object); }

  ~PersistentMappedBuffer() {
    if (m_object) {
      glBindBuffer(GLenum::GL_ARRAY_BUFFER, m_object);
      glUnmapBuffer(GLenum::GL_ARRAY_BUFFER);
      glDeleteBuffers(1, &m_object);
    }
  }

 private:
  GLuint m_object;
  T* m_memory;
};

}  // namespace renderer
