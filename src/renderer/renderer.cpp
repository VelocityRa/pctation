#include <renderer/renderer.hpp>

#include <gpu/gpu.hpp>
#include <renderer/shader.hpp>

#include <glbinding/gl/gl.h>
#include <gsl-lite.hpp>

#include <algorithm>
#include <cassert>
#include <string>

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

gpu::RGB16 Renderer::calculate_pixel_shaded(Color3 colors, BarycentricCoords bar) {
  // https://codeplea.com/triangular-interpolation

  const auto w0 = bar[0];
  const auto w1 = bar[1];
  const auto w2 = bar[2];

  auto w = (float)(w0 + w1 + w2);
  u8 r = (u8)((colors[0].r * w0 + colors[1].r * w1 + colors[2].r * w2) / w);
  u8 g = (u8)((colors[0].g * w0 + colors[1].g * w1 + colors[2].g * w2) / w);
  u8 b = (u8)((colors[0].b * w0 + colors[1].b * w1 + colors[2].b * w2) / w);

  return gpu::RGB16::fromRGB(r, g, b);
}

template <PixelRenderType RenderType>
void Renderer::draw_pixel(Position position, Color3 colors, BarycentricCoords bar) {
  gpu::RGB16 psx_color;

  switch (RenderType) {
    case PixelRenderType::SHADED: psx_color = calculate_pixel_shaded(colors, bar); break;
    case PixelRenderType::TEXTURED_PALETTED_4BIT: assert(0); break;
    case PixelRenderType::TEXTURED_PALETTED_8BIT: assert(0); break;
    case PixelRenderType::TEXTURED_16BIT: assert(0); break;
    default: assert(0);
  }

  m_gpu.set_vram_pos(position.x, position.y, psx_color.word);
}

template <PixelRenderType RenderType>
void Renderer::draw_triangle(Position3 positions, Color3 colors) {
  // Algorithm from https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
  // TODO: optimize

  auto orient_2d = [](Position a, Position b, Position c) -> s32 {
    return ((s32)b.x - (s32)a.x) * ((s32)c.y - (s32)a.y) - ((s32)b.y - (s32)a.y) * ((s32)c.x - (s32)a.x);
  };

  // Short-hands
  const auto p = positions;
  const auto v0 = p[0];
  auto v1 = p[1];
  auto v2 = p[2];

  // If CCW order, swap vertices to make it CW
  const auto is_ccw = orient_2d(p[0], p[1], p[2]) < 0;
  if (is_ccw)
    std::swap(v1, v2);

  // Compute triangle bounding box and clip against drawing area bounds
  const auto da_tl = m_gpu.m_drawing_area_top_left;
  const auto da_br = m_gpu.m_drawing_area_bottom_right;
  s16 min_x = std::max((s16)da_tl.x, std::max((s16)0, std::min({ v0.x, v1.x, v2.x })));
  s16 min_y = std::max((s16)da_tl.y, std::max((s16)0, std::min({ v0.y, v1.y, v2.y })));
  s16 max_x = std::max((s16)da_br.x, std::min((s16)gpu::VRAM_WIDTH, std::min({ v0.x, v1.x, v2.x })));
  s16 max_y = std::max((s16)da_br.y, std::min((s16)gpu::VRAM_HEIGHT, std::min({ v0.y, v1.y, v2.y })));

  // Rasterize
  Position p_iter;
  for (p_iter.y = min_y; p_iter.y <= max_y; p_iter.y++)
    for (p_iter.x = min_x; p_iter.x <= max_x; p_iter.x++) {
      // Determine barycentric coordinates
      const auto w0 = orient_2d(v1, v2, p_iter);
      auto w1 = orient_2d(v2, v0, p_iter);
      auto w2 = orient_2d(v0, v1, p_iter);

      // If p is on or inside all edges, render pixel
      if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
        if (is_ccw)
          std::swap(w1, w2);
        draw_pixel<RenderType>(p_iter, colors, { w0, w1, w2 });
      }
    }
}

void Renderer::draw_quad_shaded(Position4 positions, Color4 colors) {
  const auto p0 = Position3{ positions[0], positions[1], positions[2] };
  const auto c0 = Color3{ colors[0], colors[1], colors[2] };

  const auto p1 = Position3{ positions[1], positions[2], positions[3] };
  const auto c1 = Color3{ colors[1], colors[2], colors[3] };

  draw_triangle<PixelRenderType::SHADED>(p0, c0);
  draw_triangle<PixelRenderType::SHADED>(p1, c1);
}

void Renderer::draw_quad_mono(Position4 positions, Color color) {
  const auto p0 = Position3{ positions[0], positions[1], positions[2] };
  const auto c0 = Color3{ color, color, color };

  const auto p1 = Position3{ positions[1], positions[2], positions[3] };
  const auto c1 = Color3{ color, color, color };

  draw_triangle<PixelRenderType::SHADED>(p0, c0);
  draw_triangle<PixelRenderType::SHADED>(p1, c1);
}

// void Renderer::draw_rect_mono(Position4 positions, Color color) {
//  const auto first_row_idx = (positions[0].x + positions[0].y * VRAM_WIDTH) * 3;
//  const auto last_row_idx = (positions[2].x + positions[2].y * VRAM_WIDTH) * 3;
//  const auto row_distance = VRAM_WIDTH * 3;
//  const auto row_size = (positions[1].x - positions[0].x) * 3;
//
//  for (auto row_start_idx = first_row_idx; row_start_idx < last_row_idx; row_start_idx += row_distance)
//  {
//    const auto cur_row_end_idx = row_start_idx + row_size;
//    for (auto idx = row_start_idx; idx < cur_row_end_idx; idx += 3) {
//      set_vram_color(idx, color);
//    }
//  }
//}

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
