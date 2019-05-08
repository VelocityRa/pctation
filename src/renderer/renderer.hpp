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

constexpr u32 MAX_GP0_CMD_LEN = 32;

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
  Position operator+(const Position& rhs) const {
    // TODO: sign extend?
    return { x + rhs.x, y + rhs.y };
  }
};

struct Size {
  s16 width;
  s16 height;

  static Size from_gp0(u32 cmd) {
    return { util::sign_extend<10, s16>((s16)cmd & 0x1FF),
             util::sign_extend<10, s16>((s16)(cmd >> 16) & 0x3FF) };
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
  s16 x;
  s16 y;

  static Texcoord from_gp0(u32 cmd) { return { (u8)(cmd & 0xFF), (u8)((cmd >> 8) & 0xFF) }; }
  static Texcoord4 from_gp0(u32 cmd1, u32 cmd2, u32 cmd3, u32 cmd4) {
    return { from_gp0(cmd1), from_gp0(cmd2), from_gp0(cmd3), from_gp0(cmd4) };
  }
  Texcoord operator+(const Texcoord& rhs) const {
    // TODO: sign extend?
    return { x + rhs.x, y + rhs.y };
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
  Palette palette;
  u16 page;
  Color color;

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

// Last 3 values map to GPUSTAT.7-8 "Texture Page Colors"
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

// First byte of GP0 draw commands
union DrawCommand {
  enum class TextureMode : u8 {
    Blended = 0,
    Raw = 1,
  };
  enum class RectSize : u8 {
    SizeVariable = 0,
    Size1x1 = 1,
    Size8x8 = 2,
    Size16x16 = 3,
  };
  enum class VertexCount : u8 {
    Triple = 0,
    Quad = 1,
  };
  enum class LineCount : u8 {
    Single = 0,
    Poly = 1,
  };
  enum class Shading : u8 {
    Flat = 0,
    Gouraud = 1,
  };
  enum class PrimitiveType : u8 {
    Polygon = 1,
    Line = 2,
    Rectangle = 3,
  };

  u8 word{};

  struct Line {
    u8 : 1;
    u8 semi_transparency : 1;
    u8 : 1;
    LineCount line_count : 1;
    Shading shading : 1;
    u8 : 3;

    bool is_poly() const { return line_count == LineCount::Poly; }
    u8 get_arg_count() const {
      if (is_poly())
        return MAX_GP0_CMD_LEN - 1;
      return 2 + (shading == Shading::Gouraud ? 1 : 0);
    }

  } line;

  struct Rectangle {
    TextureMode texture_mode : 1;
    u8 semi_transparency : 1;
    u8 texture_mapping : 1;
    RectSize rect_size : 2;
    u8 : 3;

    bool is_variable_sized() const { return rect_size == RectSize::SizeVariable; }
    Size get_static_size() const {
      switch (rect_size) {
        case RectSize::Size1x1: return { 1, 1 };
        case RectSize::Size8x8: return { 8, 8 };
        case RectSize::Size16x16: return { 16, 16 };
        case RectSize::SizeVariable:
        default: LOG_ERROR("Invalid size"); return { 0, 0 };
      }
    }
    u8 get_arg_count() const {
      u8 arg_count = 1;

      if (is_variable_sized())
        arg_count += 1;

      if (texture_mapping)
        arg_count += 1;
      return arg_count;
    }
  } rectangle;

  struct Polygon {
    TextureMode texture_mode : 1;
    u8 semi_transparency : 1;
    u8 texture_mapping : 1;
    VertexCount vertex_count : 1;
    Shading shading : 1;
    u8 : 3;

    bool is_quad() const { return vertex_count == VertexCount::Quad; }
    u8 get_vertex_count() const { return is_quad() ? 4 : 3; }
    u8 get_arg_count() const {
      u8 arg_count = get_vertex_count();

      if (texture_mapping)
        arg_count *= 2;

      if (shading == Shading::Gouraud)
        arg_count += get_vertex_count() - 1;
      return arg_count;
    }
  } polygon;

  struct Flags {
    TextureMode texture_mode : 1;
    u8 semi_transparency : 1;
    u8 texture_mapped : 1;
    u8 : 1;
    Shading shading : 1;
  } flags;
};

class Renderer {
 public:
  explicit Renderer(gpu::Gpu& gpu);
  ~Renderer();

  void render();

  template <PixelRenderType RenderType>
  void draw_pixel(Position pos,
                  DrawTriVariableArgs draw_args,
                  BarycentricCoords bar,
                  DrawCommand::Flags draw_flags);

  template <PixelRenderType RenderType>
  void draw_triangle(Position3 pos, DrawTriVariableArgs draw_args, DrawCommand::Flags draw_flags);

  void draw_polygon(const DrawCommand::Polygon& polygon);
  void draw_rectangle(const DrawCommand::Rectangle& polygon);

 private:
  void draw_polygon_impl(Position4 positions,
                         Color4 colors,
                         TextureInfo tex_info,
                         bool is_quad,
                         DrawCommand::Flags draw_flags);

  TexelPos calculate_texel_pos(BarycentricCoords bar, Texcoord3 uv) const;
  static gpu::RGB16 calculate_pixel_shaded(Color3 colors, BarycentricCoords bar);
  gpu::RGB16 calculate_pixel_tex_4bit(TextureInfo tex_info, TexelPos texel_pos) const;
  gpu::RGB16 calculate_pixel_tex_16bit(TextureInfo tex_info, TexelPos texel_pos) const;

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
