#pragma once

#include <renderer/buffer.hpp>
#include <util/types.hpp>

#include <glbinding/gl/types.h>

#include <array>

using namespace gl;

namespace renderer {

struct Position;
struct Color;

using Color2 = std::array<Color, 2>;
using Position2 = std::array<Position, 2>;
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

  void draw();

  void draw_pixel(Position position, Color color);
  void draw_triangle_shaded(Position3 posistions, Color3 colors);
  void draw_quad_shaded(Position4 posistions, Color4 colors);
  void draw_quad_mono(Position4 positions, Color color);
  //  void draw_rect_mono(Position2 array, Color color);

 private:
  inline void set_vram_color(u32 vram_idx, Color color);
  static inline Color get_shaded_color(Color3 colors, u32 w0, u32 w1, u32 w2);

 private:
  // Shaders
  GLuint m_shader_program_screen{};

  // Other OpenGL objects
  GLuint m_vao;
  GLuint m_vbo;
  GLuint m_tex_screen;

  // VRAM
  std::array<u8, 1024 * 512 * 3> m_vram_fb;
};

}  // namespace renderer
