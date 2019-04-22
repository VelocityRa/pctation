#pragma once

#include <gpu/colors.hpp>
#include <renderer/buffer.hpp>
#include <util/types.hpp>

#include <glbinding/gl/types.h>

#include <array>

using namespace gl;

namespace gpu {
class Gpu;
}

namespace renderer {

struct Position;
struct Color;

using Color3 = std::array<Color, 3>;
using Color4 = std::array<Color, 4>;
using Position3 = std::array<Position, 3>;
using Position4 = std::array<Position, 4>;

struct Position {
  s16 x;
  s16 y;

  static Position from_gp0(u32 cmd) { return { (s16)cmd & 0x7FF, (s16)(cmd >> 16) & 0x7FF }; }
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

  u32 word() const { return r | g << 8 | b << 16; }
};

enum class PixelRenderType {
  SHADED,
  TEXTURED_PALETTED_4BIT,
  TEXTURED_PALETTED_8BIT,
  TEXTURED_16BIT,
};

using BarycentricCoords = std::array<s32, 3>;

class Renderer {
 public:
  explicit Renderer(gpu::Gpu& gpu);
  ~Renderer();

  void render();

  template <PixelRenderType RenderType>
  void draw_triangle(Position3 positions, Color3 colors);

  template <PixelRenderType RenderType>
  void draw_pixel(Position position, Color3 colors, BarycentricCoords bar);

  void draw_quad_shaded(Position4 posistions, Color4 colors);
  void draw_quad_mono(Position4 positions, Color color);
  //  void draw_rect_mono(Position2 array, Color color);

 private:
  static gpu::RGB16 calculate_pixel_shaded(Color3 colors, BarycentricCoords bar);

 private:
  // Shaders
  GLuint m_shader_program_screen{};

  // Other OpenGL objects
  GLuint m_vao{};
  GLuint m_vbo{};
  GLuint m_tex_screen{};

  // GPU reference
  gpu::Gpu& m_gpu;
};

}  // namespace renderer
