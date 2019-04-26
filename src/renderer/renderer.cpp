#include <renderer/renderer.hpp>

#include <gpu/gpu.hpp>
#include <renderer/shader.hpp>

#include <glbinding/gl/gl.h>
#include <gsl-lite.hpp>

#include <algorithm>
#include <cassert>
#include <numeric>
#include <string>
#include <variant>

using namespace gl;

namespace renderer {

const GLuint ATTRIB_INDEX_POSITION = 0;
const GLuint ATTRIB_INDEX_TEXCOORD = 1;

Renderer::Renderer(gpu::Gpu& gpu) : m_gpu(gpu) {
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
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_tex_screen);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gpu::VRAM_WIDTH, gpu::VRAM_HEIGHT, 0, GL_RGBA,
               GL_UNSIGNED_SHORT_1_5_5_5_REV, nullptr);

  glBindVertexArray(0);
}

Renderer::~Renderer() {
  glDeleteTextures(1, &m_tex_screen);
  glDeleteBuffers(1, &m_vbo);
  glDeleteVertexArrays(1, &m_vao);
  glDeleteProgram(m_shader_program_screen);
}

template <PixelRenderType RenderType>
void Renderer::draw_pixel(Position pos, DrawTriVariableArgs draw_args, BarycentricCoords bar) {
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
    case PixelRenderType::TEXTURED_PALETTED_4BIT: {
      out_color = calculate_pixel_tex_4bit(tex_info, texel);
      break;
    }
    case PixelRenderType::TEXTURED_PALETTED_8BIT: assert(0); break;
    case PixelRenderType::TEXTURED_16BIT: assert(0); break;
    default: assert(0);
  }

  // Don't write to VRAM if drawing a texture, or if (TODO) the semi-transparency bit is enabled
  if (is_textured && out_color.word == 0x0000)
    return;

  // Apply texture color or (TODO) shading
  if (is_textured) {
    auto tex_color = gpu::RGB32::from_word(tex_info.color.word());
    out_color *= tex_color.r / 255.f * 2.f;
  }

  m_gpu.set_vram_pos(pos.x, pos.y, out_color.word);
}

gpu::RGB16 Renderer::calculate_pixel_shaded(Color3 colors, BarycentricCoords bar) {
  // https://codeplea.com/triangular-interpolation

  auto w = (float)(bar.a + bar.b + bar.c);
  u8 r = (u8)((colors[0].r * bar.a + colors[1].r * bar.b + colors[2].r * bar.c) / w);
  u8 g = (u8)((colors[0].g * bar.a + colors[1].g * bar.b + colors[2].g * bar.c) / w);
  u8 b = (u8)((colors[0].b * bar.a + colors[1].b * bar.b + colors[2].b * bar.c) / w);

  return gpu::RGB16::from_RGB(r, g, b);
}

gpu::RGB16 Renderer::calculate_pixel_tex_4bit(TextureInfo tex_info, TexelPos texel_pos) {
  // HACKY: Use GpuStatus type becuse the first 11 bits are the same
  const auto texpage = gpu::GpuStatus{ tex_info.page };

  const auto index_x = texel_pos.x / 4 + texpage.tex_base_x();
  const auto index_y = texel_pos.y + texpage.tex_base_y();

  u16 index = m_gpu.get_vram_pos(index_x, index_y);
  const auto index_shift = (texel_pos.x & 0b11) << 2;
  u16 entry = (index >> index_shift) & 0xF;

  const auto clut_x = tex_info.clut.x() + entry;
  const auto clut_y = tex_info.clut.y();

  u16 color = m_gpu.get_vram_pos(clut_x, clut_y);

  return gpu::RGB16::from_word(color);
}

TexelPos Renderer::calculate_texel_pos(BarycentricCoords bar, Texcoord3 uv) {
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
void Renderer::draw_triangle(Position3 positions,
                             DrawTriVariableArgs draw_args,
                             QuadTriangleIndex triangle_index) {
  // Algorithm from https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
  // TODO: optimize

  // Determine orientation of 3 points in 2D space
  auto orient_2d = [](Position a, Position b, Position c) -> s32 {
    return ((s32)b.x - a.x) * ((s32)c.y - a.y) - ((s32)b.y - a.y) * ((s32)c.x - a.x);
  };

  if (RenderType != PixelRenderType::SHADED) {
    auto& tex_info = std::get<TextureInfo>(draw_args);
    tex_info.update_active_triangle(triangle_index);
  }

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
        draw_pixel<RenderType>(p_iter, draw_args, normalized_bar);
      }
    }
}

void Renderer::draw_triangle_shaded(Position3 positions,
                                    Color3 colors,
                                    QuadTriangleIndex triangle_index) {
  draw_triangle<PixelRenderType::SHADED>(positions, colors, triangle_index);
}

void Renderer::draw_quad_shaded(Position4 positions, Color4 colors) {
  const auto p0 = Position3{ positions[0], positions[1], positions[2] };
  const auto c0 = Color3{ colors[0], colors[1], colors[2] };

  const auto p1 = Position3{ positions[1], positions[2], positions[3] };
  const auto c1 = Color3{ colors[1], colors[2], colors[3] };

  draw_triangle_shaded(p0, c0, QuadTriangleIndex::First);
  draw_triangle_shaded(p1, c1, QuadTriangleIndex::Second);
}

void Renderer::draw_quad_mono(Position4 positions, Color color) {
  const auto p0 = Position3{ positions[0], positions[1], positions[2] };
  const auto c0 = Color3{ color, color, color };

  const auto p1 = Position3{ positions[1], positions[2], positions[3] };
  const auto c1 = Color3{ color, color, color };

  draw_triangle_shaded(p0, c0, QuadTriangleIndex::First);
  draw_triangle_shaded(p1, c1, QuadTriangleIndex::Second);
}

void Renderer::draw_quad_textured(Position4 positions, TextureInfo tex_info) {
  const auto p0 = Position3{ positions[0], positions[1], positions[2] };
  const auto p1 = Position3{ positions[1], positions[2], positions[3] };

  draw_triangle<PixelRenderType::TEXTURED_PALETTED_4BIT>(p0, tex_info, QuadTriangleIndex::First);
  draw_triangle<PixelRenderType::TEXTURED_PALETTED_4BIT>(p1, tex_info, QuadTriangleIndex::Second);
}

void Renderer::render() {
  // Bind needed state
  glBindVertexArray(m_vao);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_tex_screen);
  glUseProgram(m_shader_program_screen);

  // Upload screen texture
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, gpu::VRAM_WIDTH, gpu::VRAM_HEIGHT, GL_RGBA,
                  GL_UNSIGNED_SHORT_1_5_5_5_REV, (const void*)m_gpu.vram().data());

  // Draw screen
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

}  // namespace renderer
