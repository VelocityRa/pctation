#include <renderer/rasterizer.hpp>

#include <gpu/gpu.hpp>

#include <glm/vec3.hpp>
#include <gsl-lite.hpp>

#include <algorithm>
#include <cassert>
#include <numeric>
#include <string>
#include <variant>

namespace renderer {
namespace rasterizer {

PixelRenderType tex_page_col_to_render_type(u8 tex_page_colors);

template <PixelRenderType RenderType>
void Rasterizer::draw_pixel(Position pos,
                            DrawTriVariableArgs draw_args,
                            BarycentricCoords bar,
                            DrawCommand::Flags draw_flags) {
  // Texture stuff, unused for SHADED render type
  TextureInfo tex_info{};
  TexelPos texel{};

  constexpr bool is_textured = RenderType != PixelRenderType::SHADED;

  if (is_textured) {
    tex_info = std::get<TextureInfo>(draw_args);
    texel = calculate_texel_pos(bar, tex_info.uv_active);
  }

  gpu::RGB16 out_color;

  switch (RenderType) {
    case PixelRenderType::SHADED: {
      const auto color = std::get<Color3>(draw_args);
      out_color = calculate_pixel_shaded(color, bar);
      break;
    }
    case PixelRenderType::TEXTURED_PALETTED_4BIT:
      out_color = calculate_pixel_tex_4bit(tex_info, texel);
      break;
    case PixelRenderType::TEXTURED_PALETTED_8BIT: assert(0); break;
    case PixelRenderType::TEXTURED_16BIT: out_color = calculate_pixel_tex_16bit(tex_info, texel); break;
    default: assert(0);
  }

  // Don't write to VRAM if drawing a texture, or if (TODO) the semi-transparency bit is enabled
  if (is_textured && out_color.word == 0x0000)
    return;

  const auto is_raw = draw_flags.texture_mode == DrawCommand::TextureMode::Raw;

  // Apply texture color or (TODO) shading
  if (is_textured && !is_raw) {
    glm::vec3 brightness;
    if (draw_flags.shading == DrawCommand::Shading::Flat) {
      brightness = gpu::RGB32::from_word(tex_info.color.word()).to_vec();
    } else if (draw_flags.shading == DrawCommand::Shading::Gouraud) {
      // TODO: shading
      brightness = glm::vec3(0.5);
    }
    out_color *= brightness * 2.f;
  }

  m_gpu.set_vram_pos(pos.x, pos.y, out_color.word, false);
}

gpu::RGB16 Rasterizer::calculate_pixel_shaded(Color3 colors, BarycentricCoords bar) {
  // https://codeplea.com/triangular-interpolation

  auto w = (float)(bar.a + bar.b + bar.c);
  u8 r = (u8)((colors[0].r * bar.a + colors[1].r * bar.b + colors[2].r * bar.c) / w);
  u8 g = (u8)((colors[0].g * bar.a + colors[1].g * bar.b + colors[2].g * bar.c) / w);
  u8 b = (u8)((colors[0].b * bar.a + colors[1].b * bar.b + colors[2].b * bar.c) / w);

  return gpu::RGB16::from_RGB(r, g, b);
}

gpu::RGB16 Rasterizer::calculate_pixel_tex_4bit(TextureInfo tex_info, TexelPos texel_pos) const {
  const auto texpage = gpu::Gp0DrawMode{ tex_info.page };

  const auto index_x = texel_pos.x / 4 + texpage.tex_base_x();
  const auto index_y = texel_pos.y + texpage.tex_base_y();

  u16 index = m_gpu.get_vram_pos(index_x, index_y);

  const auto index_shift = (texel_pos.x & 0b11) * 4;
  u16 entry = (index >> index_shift) & 0xF;

  const auto clut_x = tex_info.palette.x() + entry;
  const auto clut_y = tex_info.palette.y();

  u16 color = m_gpu.get_vram_pos(clut_x, clut_y);

  return gpu::RGB16::from_word(color);
}

gpu::RGB16 Rasterizer::calculate_pixel_tex_16bit(TextureInfo tex_info, TexelPos texel_pos) const {
  const auto texpage = gpu::Gp0DrawMode{ tex_info.page };

  const auto color_x = texel_pos.x + texpage.tex_base_x();
  const auto color_y = texel_pos.y + texpage.tex_base_y();

  u16 color = m_gpu.get_vram_pos(color_x, color_y);

  return gpu::RGB16::from_word(color);
}

TexelPos Rasterizer::calculate_texel_pos(BarycentricCoords bar, Texcoord3 uv) const {
  TexelPos texel;

  texel.x = (s32)(bar.a * uv[0].x + bar.b * uv[1].x + bar.c * uv[2].x);
  texel.y = (s32)(bar.a * uv[0].y + bar.b * uv[1].y + bar.c * uv[2].y);

  // Texture repeats
  texel.x %= 256;
  texel.y %= 256;

  // Texture mask
  const auto tex_win = m_gpu.m_tex_window;
  texel.x = (texel.x & ~(tex_win.tex_window_mask_x * 8)) |
            ((tex_win.tex_window_off_x & tex_win.tex_window_mask_x) * 8);
  texel.y = (texel.y & ~(tex_win.tex_window_mask_y * 8)) |
            ((tex_win.tex_window_off_y & tex_win.tex_window_mask_y) * 8);

  return texel;
}

template <PixelRenderType RenderType>
void Rasterizer::draw_triangle(Position3 positions,
                               DrawTriVariableArgs draw_args,
                               DrawCommand::Flags draw_flags) {
  // Algorithm from https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
  // TODO: optimize

  // Determine orientation of 3 points in 2D space
  auto orient_2d = [](Position a, Position b, Position c) -> s32 {
    return ((s32)b.x - a.x) * ((s32)c.y - a.y) - ((s32)b.y - a.y) * ((s32)c.x - a.x);
  };

  // Apply drawing offset
  const auto drawing_offset = m_gpu.m_drawing_offset;

  for (auto i = 0; i < 3; ++i) {
    positions[i].x += drawing_offset.x;
    positions[i].y += drawing_offset.y;
  }

  // Short-hands
  const auto v0 = positions[0];
  auto v1 = positions[1];
  auto v2 = positions[2];

  // If CCW order, swap vertices (and tex coords) to make it CW
  const auto area = orient_2d(v0, v1, v2);
  if (area == 0)  // TODO: Is this needed?
    return;
  const auto is_ccw = area < 0;

  if (is_ccw) {
    std::swap(v1, v2);

    if (RenderType != PixelRenderType::SHADED) {
      auto& tex_info = std::get<TextureInfo>(draw_args);
      tex_info.swap_active_uv_coords();
    }
  }

  // Compute triangle bounding box and clip against drawing area bounds
  const auto da_left = m_gpu.m_drawing_area_top_left.y;
  const auto da_top = m_gpu.m_drawing_area_top_left.x;
  const auto da_right = m_gpu.m_drawing_area_bottom_right.x;
  const auto da_bottom = m_gpu.m_drawing_area_bottom_right.y;
  s16 min_x = std::max((s16)da_left, std::max((s16)0, std::min({ v0.x, v1.x, v2.x })));
  s16 min_y = std::max((s16)da_top, std::max((s16)0, std::min({ v0.y, v1.y, v2.y })));
  s16 max_x = std::min((s16)da_right, std::min((s16)gpu::VRAM_WIDTH, std::max({ v0.x, v1.x, v2.x })));
  s16 max_y = std::min((s16)da_bottom, std::min((s16)gpu::VRAM_HEIGHT, std::max({ v0.y, v1.y, v2.y })));

  // Rasterize
  Position p_iter;
  for (p_iter.y = min_y; p_iter.y < max_y; p_iter.y++)
    for (p_iter.x = min_x; p_iter.x < max_x; p_iter.x++) {
      // Determine barycentric coordinates
      const auto w0 = orient_2d(v1, v2, p_iter);
      auto w1 = orient_2d(v2, v0, p_iter);
      auto w2 = orient_2d(v0, v1, p_iter);

      // If p is on or inside all edges, render pixel
      if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
        if (is_ccw)
          std::swap(w1, w2);
        const auto area_abs = std::abs(area);
        const auto normalized_bar =
            BarycentricCoords{ w0 / (float)area_abs, w1 / (float)area_abs, w2 / (float)area_abs };
        draw_pixel<RenderType>(p_iter, draw_args, normalized_bar, draw_flags);
      }
    }
}

void Rasterizer::draw_polygon_impl(Position4 positions,
                                   Color4 colors,
                                   TextureInfo tex_info,
                                   bool is_quad,
                                   DrawCommand::Flags draw_flags) {
  // Consolidate args data and call appropriate drawing functions
  const auto end_tri_idx = is_quad ? QuadTriangleIndex::Second : QuadTriangleIndex::First;
  const auto is_textured = draw_flags.texture_mapped;

  const auto texpage = gpu::Gp0DrawMode{ tex_info.page };
  auto pixel_render_type = tex_page_col_to_render_type(texpage.tex_page_colors);

  Position3 tri_positions = { positions[0], positions[1], positions[2] };
  Color3 tri_colors = { colors[0], colors[1], colors[2] };

  QuadTriangleIndex tri_idx = QuadTriangleIndex::First;
  while (tri_idx <= end_tri_idx) {
    if (is_textured) {                             // Textured
      if (tri_idx == QuadTriangleIndex::Second) {  // rendering second triangle
        tri_positions = { positions[1], positions[2], positions[3] };
      }
      tex_info.update_active_triangle(tri_idx);

      switch (pixel_render_type) {
        case PixelRenderType::TEXTURED_PALETTED_4BIT:
          draw_triangle<PixelRenderType::TEXTURED_PALETTED_4BIT>(tri_positions, tex_info, draw_flags);
          break;
        case PixelRenderType::TEXTURED_PALETTED_8BIT:
          LOG_ERROR("Unimplemented TEXTURED_PALETTED_8BIT draw cmd");
          break;
        case PixelRenderType::TEXTURED_16BIT:
          draw_triangle<PixelRenderType::TEXTURED_16BIT>(tri_positions, tex_info, draw_flags);
          break;
        case PixelRenderType::SHADED:
        default: LOG_ERROR("Invalid textured PixelRenderType"); assert(0);
      }
    } else {                                       // Non-textured
      if (tri_idx == QuadTriangleIndex::Second) {  // rendering second triangle
        tri_positions = { positions[1], positions[2], positions[3] };
        tri_colors = { colors[1], colors[2], colors[3] };
      }
      draw_triangle<PixelRenderType::SHADED>(tri_positions, tri_colors, draw_flags);
    }

    tri_idx = (QuadTriangleIndex)((u32)tri_idx + 1);
  }
}

void Rasterizer::extract_draw_data_polygon(const DrawCommand::Polygon& polygon,
                                           const std::vector<u32>& gp0_cmd,
                                           Position4& positions,
                                           Color4& colors,
                                           TextureInfo& tex_info) const {
  const auto vertex_count = polygon.get_vertex_count();
  u8 arg_idx = 1;

  // Gather data from command words
  for (auto v_idx = 0; v_idx < vertex_count; ++v_idx) {
    positions[v_idx] = Position::from_gp0(gp0_cmd[arg_idx++]);

    if (polygon.texture_mode == DrawCommand::TextureMode::Blended &&
        (polygon.shading == DrawCommand::Shading::Flat || v_idx == 0))
      colors[v_idx] = Color::from_gp0(gp0_cmd[0]);
    if (polygon.texture_mapping) {
      if (v_idx == 0)
        tex_info.palette = Palette::from_gp0(gp0_cmd[arg_idx]);
      if (v_idx == 1)
        tex_info.page = (gp0_cmd[arg_idx] >> 16) & 0xFFFF;
      tex_info.uv[v_idx] = Texcoord::from_gp0(gp0_cmd[arg_idx++]);
    }
    if (polygon.shading == DrawCommand::Shading::Gouraud && v_idx < vertex_count - 1)
      colors[v_idx + 1] = Color::from_gp0(gp0_cmd[arg_idx++]);
  }
  tex_info.color = colors[0];

  // TODO: semi transparency
  // TODO: raw textures
  // TODO: dithering
}

void Rasterizer::draw_polygon(const DrawCommand::Polygon& polygon) {
  const auto gp0_cmd = m_gpu.gp0_cmd();

  Position4 positions{};
  Color4 colors{};
  TextureInfo tex_info{};

  extract_draw_data_polygon(polygon, gp0_cmd, positions, colors, tex_info);

  draw_polygon_impl(positions, colors, tex_info, polygon.is_quad(), *(DrawCommand::Flags*)&polygon);
}

void Rasterizer::extract_draw_data_rectangle(const DrawCommand::Rectangle& rectangle,
                                             const std::vector<u32>& gp0_cmd,
                                             Position4& positions,
                                             Color4& colors,
                                             TextureInfo& tex_info,
                                             Size& size) const {
  const auto is_textured = rectangle.texture_mapping;

  u8 arg_idx = 1;

  // Gather data from command words
  colors[0] = colors[1] = colors[2] = colors[3] = Color::from_gp0(gp0_cmd[0]);
  positions[0] = Position::from_gp0(gp0_cmd[arg_idx++]);

  if (is_textured) {
    tex_info.palette = Palette::from_gp0(gp0_cmd[arg_idx]);
    tex_info.page = (u16)m_gpu.m_draw_mode.word;
    tex_info.color = colors[0];
    tex_info.uv[0] = Texcoord::from_gp0(gp0_cmd[arg_idx++]);
  }
  if (rectangle.is_variable_sized())
    size = Size::from_gp0(gp0_cmd[arg_idx++]);
  else
    size = rectangle.get_static_size();

  positions[1] = positions[0] + Position{ size.width, (s16)0 };
  positions[2] = positions[0] + Position{ (s16)0, size.height };
  positions[3] = positions[0] + Position{ size.width, size.height };

  if (is_textured) {
    auto& uv = tex_info.uv;
    uv[1] = uv[0] + Texcoord{ size.width, 0 };
    uv[2] = uv[0] + Texcoord{ 0, size.height };
    uv[3] = uv[0] + Texcoord{ size.width, size.height };
  }
}

void Rasterizer::draw_rectangle(const DrawCommand::Rectangle& rectangle) {
  Position4 positions{};
  Color4 colors{};
  TextureInfo tex_info{};
  Size size{};

  extract_draw_data_rectangle(rectangle, m_gpu.gp0_cmd(), positions, colors, tex_info, size);
  // TODO: semi transparency
  // TODO: raw textures
  const auto is_quad = true;
  draw_polygon_impl(positions, colors, tex_info, is_quad, *(DrawCommand::Flags*)&rectangle);
}

PixelRenderType tex_page_col_to_render_type(u8 tex_page_colors) {
  switch (tex_page_colors) {
    case 0: return PixelRenderType::TEXTURED_PALETTED_4BIT;
    case 1: return PixelRenderType::TEXTURED_PALETTED_8BIT;
    case 2: return PixelRenderType::TEXTURED_16BIT;
    case 3: return PixelRenderType::TEXTURED_16BIT;
  }
  return PixelRenderType::SHADED;  // unreachable
}

}  // namespace rasterizer
}  // namespace renderer