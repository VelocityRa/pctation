#pragma once

#include <gpu/colors.hpp>
#include <renderer/buffer.hpp>
#include <util/bit_utils.hpp>
#include <util/log.hpp>
#include <util/types.hpp>

#include <glbinding/gl/types.h>

#include <algorithm>
#include <array>
#include <variant>

using namespace gl;

namespace gpu {
class Gpu;
}

namespace renderer {

enum class QuadTriangleIndex {
  None,
  First,
  Second,
};

struct Color;
struct Position;
struct Texcoord;

using Color3 = std::array<Color, 3>;
using Color4 = std::array<Color, 4>;
using Position3 = std::array<Position, 3>;
using Position4 = std::array<Position, 4>;
using Texcoord3 = std::array<Texcoord, 3>;
using Texcoord4 = std::array<Texcoord, 4>;

struct Position {
  s16 x;
  s16 y;

  static Position from_gp0(u32 cmd) {
    return { util::sign_extend<10, s16>((s16)cmd & 0x7FF),
             util::sign_extend<10, s16>((s16)(cmd >> 16) & 0x7FF) };
  }
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

struct Texcoord {
  u8 x;
  u8 y;

  static Texcoord from_gp0(u32 cmd) { return { (u8)(cmd & 0xFF), (u8)((cmd >> 8) & 0xFF) }; }
  static Texcoord4 from_gp0(u32 cmd1, u32 cmd2, u32 cmd3, u32 cmd4) {
    return { from_gp0(cmd1), from_gp0(cmd2), from_gp0(cmd3), from_gp0(cmd4) };
  }
};

union Palette {
  struct {
    u16 _x : 6;
    u16 _y : 9;
    u16 _pad : 1;
  };
  static Palette from_gp0(u32 cmd) {
    Palette p;
    p.word = (cmd >> 16) & 0xFFFF;
    return p;
  }
  u16 x() const { return _x * 16; }
  u16 y() const { return _y; }

  u16 word;
};

struct TextureInfo {
  Texcoord4 uv;
  Texcoord3 uv_active;  // UVs of currently rendering triangle
  Palette clut;
  u16 page;
  Color color;

  static TextureInfo from_gp0(u32 cmd1, u32 cmd2, u32 cmd3, u32 cmd4, u32 cmd5 = 0) {
    TextureInfo ti;
    ti.color = Color::from_gp0(cmd1);
    ti.uv = Texcoord::from_gp0(cmd2, cmd3, cmd4, cmd5);
    ti.clut = Palette::from_gp0(cmd2);
    ti.page = (cmd3 >> 16) & 0xFFFF;
    return ti;
  }
  void update_active_triangle(QuadTriangleIndex triangle_index) {
    switch (triangle_index) {
      case QuadTriangleIndex::First: uv_active = { uv[0], uv[1], uv[2] }; break;
      case QuadTriangleIndex::Second: uv_active = { uv[1], uv[3], uv[2] }; break;
      case QuadTriangleIndex::None:
        /* fallthrough */
      default:
        LOG_ERROR("Invalid QuadTriangleIndex");
        assert(0);
        break;
    }
  }

  // Swap last two UV coords to make them match swapped vertices (for previously CCW-ordered triangles)
  void swap_active_uv_coords() { std::swap(uv_active[1], uv_active[2]); }
};

// Tagged union for draw command arguments that vary between command types
// For example, all of them have Positions (so we always pass those), but
// the existance of color and texture info arguments depend on the type of
// command.
using DrawTriVariableArgs = std::variant<Color3, TextureInfo>;

enum class PixelRenderType {
  SHADED,
  TEXTURED_PALETTED_4BIT,
  TEXTURED_PALETTED_8BIT,
  TEXTURED_16BIT,
};

struct BarycentricCoords {
  float a;
  float b;
  float c;
};

struct TexelPos {
  s32 x;
  s32 y;
};

class Renderer {
 public:
  explicit Renderer(gpu::Gpu& gpu);
  ~Renderer();

  void render();

  // Pixel drawing
  template <PixelRenderType RenderType>
  void draw_pixel(Position pos, DrawTriVariableArgs draw_args, BarycentricCoords bar);

  // Triangle drawing
  template <PixelRenderType RenderType>
  void draw_triangle(Position3 pos,
                     DrawTriVariableArgs draw_args,
                     QuadTriangleIndex triangle_index = QuadTriangleIndex::None);
  void draw_triangle_shaded(Position3 positions,
                            Color3 colors,
                            QuadTriangleIndex triangle_index = QuadTriangleIndex::None);

  // Quad drawing
  void draw_quad_mono(Position4 positions, Color color);
  void draw_quad_shaded(Position4 posistions, Color4 colors);
  void draw_quad_textured(Position4 positions, TextureInfo tex_info);

  //  void draw_rect_mono(Position2 array, Color color);

 private:
  TexelPos calculate_texel_pos(BarycentricCoords bar, Texcoord3 uv);

  static gpu::RGB16 calculate_pixel_shaded(Color3 colors, BarycentricCoords bar);
  gpu::RGB16 calculate_pixel_tex_4bit(TextureInfo tex_info, TexelPos texel_pos);

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
