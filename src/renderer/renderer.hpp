#pragma once

#include <renderer/buffer.hpp>
#include <util/types.hpp>

#include <glbinding/gl/types.h>

#include <array>

using namespace gl;

namespace renderer {

struct Position;
struct Color;

using Color3 = std::array<Color, 3>;
using Position3 = std::array<Position, 3>;
using Color4 = std::array<Color, 4>;
using Position4 = std::array<Position, 4>;

struct Position {
  s16 x;
  s16 y;

  static Position from_gp0(u32 cmd) { return { (s16)cmd, (s16)(cmd >> 16) }; }
  static Position3 from_gp0(u32 cmd, u32 cmd2, u32 cmd3) {
    return { from_gp0(cmd), from_gp0(cmd2), from_gp0(cmd3) };
  }
  static Position4 from_gp0(u32 cmd, u32 cmd2, u32 cmd3, u32 cmd4) {
    return { from_gp0(cmd), from_gp0(cmd2), from_gp0(cmd3), from_gp0(cmd4) };
  }
};

struct Color {
  u8 r;
  u8 g;
  u8 b;

  static Color from_gp0(u32 cmd) { return { (u8)cmd, (u8)(cmd >> 8), (u8)(cmd >> 16) }; }
  static Color3 from_gp0(u32 cmd, u32 cmd2, u32 cmd3) {
    return { from_gp0(cmd), from_gp0(cmd2), from_gp0(cmd3) };
  }
  static Color4 from_gp0(u32 cmd, u32 cmd2, u32 cmd3, u32 cmd4) {
    return { from_gp0(cmd), from_gp0(cmd2), from_gp0(cmd3), from_gp0(cmd4) };
  }
};

class Renderer {
 public:
  Renderer();
  ~Renderer();

  void push_triangle(Position3 posistions, Color3 colors);
  void push_quad(Position4 posistions, Color4 colors);
  void draw();

 private:
  static constexpr u32 VERTEX_BUFFER_LEN = 64 * 1024;
  // Vertex buffers
  renderer::PersistentMappedBuffer<Position, VERTEX_BUFFER_LEN> m_vb_positions;
  renderer::PersistentMappedBuffer<Color, VERTEX_BUFFER_LEN> m_vb_colors;
  // Vertex count in the buffers, the index we use to write to them
  u32 m_vertex_count{};

  // Shaders
  GLuint m_shader_program_gouraud{};

  // VAO
  GLuint m_vao;
};

}  // namespace renderer
