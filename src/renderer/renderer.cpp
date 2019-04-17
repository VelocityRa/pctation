#include <renderer/renderer.hpp>

#include <renderer/shader.hpp>

#include <glbinding/gl/gl.h>
#include <gsl-lite.hpp>

#include <algorithm>
#include <string>

using namespace gl;

namespace renderer {

const GLuint ATTRIB_INDEX_POSITION = 0;
const GLuint ATTRIB_INDEX_TEXCOORD = 1;

Renderer::Renderer() {
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

  const float vertices[] = {
    //  Position      Texcoords
    -1.f, 1.f,  0.0f, 0.0f,  // Top-left
    -1.f, -.5f, 0.0f, 1.0f,  // Bottom-left
    .5f,  1.f,  1.0f, 0.0f,  // Top-right
    .5f,  -.5f, 1.0f, 1.0f,  // Bottom-right
  };

  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  const auto vertex_stride = 4 * sizeof(float);
  const auto position_offset = 0 * sizeof(float);
  const auto texcoord_offset = 2 * sizeof(float);

  glVertexAttribPointer(ATTRIB_INDEX_POSITION, 2, GL_FLOAT, GL_FALSE, vertex_stride,
                        (const void*)position_offset);
  glEnableVertexAttribArray(ATTRIB_INDEX_POSITION);

  glVertexAttribPointer(ATTRIB_INDEX_TEXCOORD, 2, GL_FLOAT, GL_FALSE, vertex_stride,
                        (const void*)texcoord_offset);
  glEnableVertexAttribArray(ATTRIB_INDEX_TEXCOORD);

  // Generate and configure screen texture
  glGenTextures(1, &m_tex_screen);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_tex_screen);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 512, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

  glBindVertexArray(0);
}

Renderer::~Renderer() {
  glDeleteTextures(1, &m_tex_screen);
  glDeleteBuffers(1, &m_vbo);
  glDeleteVertexArrays(1, &m_vao);
  glDeleteProgram(m_shader_program_screen);
}

void Renderer::draw_pixel(Position position, Color color) {
  const auto idx = (position.x + position.y * 1024) * 3;

  set_vram_color(idx, color);
}

Color Renderer::get_shaded_color(Color3 colors, u32 w0, u32 w1, u32 w2) {
  // https://codeplea.com/triangular-interpolation

  auto w = (float)(w0 + w1 + w2);
  u8 r = (u8)((colors[0].r * w0 + colors[1].r * w1 + colors[2].r * w2) / w);
  u8 g = (u8)((colors[0].g * w0 + colors[1].g * w1 + colors[2].g * w2) / w);
  u8 b = (u8)((colors[0].b * w0 + colors[1].b * w1 + colors[2].b * w2) / w);

  return { r, g, b };
}

void Renderer::draw_triangle_shaded(Position3 positions, Color3 colors) {
  // Algorithm from https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/

  auto orient_2d = [](Position a, Position b, Position c) -> s32 {
    return ((s32)b.x - (s32)a.x) * ((s32)c.y - (s32)a.y) - ((s32)b.y - (s32)a.y) * ((s32)c.x - (s32)a.x);
  };

  // Short-hands
  const auto p = positions;
  const auto v0 = p[0];
  auto v1 = p[1];
  auto v2 = p[2];

  // If CCW order, swap vertices to make it CW
  const auto order = orient_2d(p[0], p[1], p[2]);
  if (order < 0)
    std::swap(v1, v2);

  // Compute triangle bounding box
  auto min_x = std::min({ v0.x, v1.x, v2.x });
  auto min_y = std::min({ v0.y, v1.y, v2.y });
  auto max_x = std::max({ v0.x, v1.x, v2.x });
  auto max_y = std::max({ v0.y, v1.y, v2.y });

  // Clip against screen bounds
  // TODO: implement display area
  min_x = std::max(min_x, (s16)0);
  min_y = std::max(min_y, (s16)0);
  max_x = std::min(max_x, (s16)(1024 - 1));
  max_y = std::min(max_y, (s16)(512 - 1));

  // Rasterize
  Position p_iter;
  for (p_iter.y = min_y; p_iter.y <= max_y; p_iter.y++)
    for (p_iter.x = min_x; p_iter.x <= max_x; p_iter.x++) {
      // Determine barycentric coordinates
      const auto w0 = orient_2d(v1, v2, p_iter);
      const auto w1 = orient_2d(v2, v0, p_iter);
      const auto w2 = orient_2d(v0, v1, p_iter);

      // If p is on or inside all edges, render pixel
      if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
        const auto shaded_color = get_shaded_color(colors, w0, w1, w2);
        draw_pixel(p_iter, shaded_color);  // todo
      }
    }
}

void Renderer::draw_quad_shaded(Position4 positions, Color4 colors) {
  const auto p0 = Position3{ positions[0], positions[1], positions[2] };
  const auto c0 = Color3{ colors[0], colors[1], colors[2] };

  const auto p1 = Position3{ positions[1], positions[2], positions[3] };
  const auto c1 = Color3{ colors[1], colors[2], colors[3] };

  draw_triangle_shaded(p0, c0);
  draw_triangle_shaded(p1, c1);
}

void Renderer::draw_quad_mono(Position4 positions, Color color) {
  const auto p0 = Position3{ positions[0], positions[1], positions[2] };
  const auto c0 = Color3{ color, color, color };

  const auto p1 = Position3{ positions[1], positions[2], positions[3] };
  const auto c1 = Color3{ color, color, color };

  draw_triangle_shaded(p0, c0);
  draw_triangle_shaded(p1, c1);
}

// void Renderer::draw_rect_mono(Position4 positions, Color color) {
//  const auto first_row_idx = (positions[0].x + positions[0].y * 1024) * 3;
//  const auto last_row_idx = (positions[2].x + positions[2].y * 1024) * 3;
//  const auto row_distance = 1024 * 3;
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

void Renderer::vram_write(u16 x, u16 y, u16 val) {
  Ensures(x <= 1024);
  Ensures(y <= 512);
  const Framebuffer15bitColor color{ val };

  draw_pixel({ (s16)x, (s16)y }, { (u8)(color.r * 8), (u8)(color.g * 8), (u8)(color.b * 8) });
}

void Renderer::set_vram_color(u32 vram_idx, Color color) {
  m_vram_fb[vram_idx + 0] = color.r;
  m_vram_fb[vram_idx + 1] = color.g;
  m_vram_fb[vram_idx + 2] = color.b;
}

void Renderer::render() {
  // Bind needed state
  glBindVertexArray(m_vao);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_tex_screen);
  glUseProgram(m_shader_program_screen);

  // Upload screen texture
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1024, 512, GL_RGB, GL_UNSIGNED_BYTE, m_vram_fb.data());

  // Draw screen
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

}  // namespace renderer
