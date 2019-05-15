#pragma once

#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>
#include <gsl-lite.hpp>
#include <util/types.hpp>

namespace cpu {
class Cpu;
class Instruction;
}  // namespace cpu

namespace cpu {
namespace gte {

using vec2_s16 = glm::vec<2, s16, glm::defaultp>;
using vec2_s32 = glm::vec<2, s32, glm::defaultp>;
using vec3_s16 = glm::vec<3, s16, glm::defaultp>;
using vec3_s32 = glm::vec<3, s32, glm::defaultp>;

using mat3x3_s16 = glm::mat<3, 3, s16, glm::defaultp>;

using col4_u8 = glm::vec<4, u8, glm::defaultp>;
using col3_s32 = glm::vec<3, s32, glm::defaultp>;

inline col4_u8 col4_from_word(u32 val) {
  // TODO: verify endianness
  col4_u8 out;
  out.r = val;
  out.g = val >> 8;
  out.b = val >> 16;
  out.a = val >> 24;
  return out;
}

inline u32 col4_to_word(col4_u8 c) {
  return (u32)c.r | (u32)c.g << 8 | (u32)c.b << 16 | (u32)c.a << 24;
}

class Gte {
 public:
  explicit Gte(cpu::Cpu& cpu) : m_cpu(cpu) {}

  u32 load_data_reg(u32 src_reg);
  u32 load_control_reg(u32 src_reg);
  void store_data_reg(u32 dest_reg, u32 val);
  void store_control_reg(u32 dest_reg, u32 val);

 private:
  //
  // Registers
  //

  // Data Registers
  vec3_s16 v[3];        // [0-5] Vector 0 - 3
  col4_u8 rgbc;         // [6] Color/code values
  u16 avg_z;            // [7] Average Z values (for Ordering Table)
  s16 ir[4];            // [8-11] 16bit Accumulator (0: Interpolate, 1-3: Vector)
  vec2_s16 s_xy[4];     // [12-15] Screen XY-coordinate FIFO  (3 stages)
  u16 s_z[4];           // [16-19] Screen Z-coordinate FIFO   (4 stages)
  col4_u8 rgb_fifo[3];  // [20-22] Color CRGB-code/color FIFO (3 stages)
  col4_u8 res;          // [23] Prohibited (?)
  s32 mac[4];           // [24-27] 32bit Maths Accumulators (0: Value, 1-3: Vector)
  u16 rgb_conv;         // [28-29] Convert RGB Color (48bit vs 15bit)
  s32 lzcs;             // Count Leading-Zeroes/Ones (sign bits)
  s32 lzcr;             // Count Leading-Zeroes/Ones (sign bits)

  // Control Registers
  mat3x3_s16 rot_mat{};            // [0-4] Rotation matrix
  vec3_s32 trans_vec{};            // [5-7] Translation vector
  mat3x3_s16 light_mat{};          // [8-12] Light source matrix
  col3_s32 bg_col{};               // [13-15] Background color
  mat3x3_s16 light_col_src_mat{};  // [16-20] Light color matrix source
  vec3_s32 far_color{};            // [21-23] Far color
  vec2_s32 screen_offset{};        // [24-25] Screen offset
  u16 h{};                         // [26] Projection plane distance
  s16 dqa{};                       // [27] Depth queing parameter A (coeff)
  s16 dqb{};                       // [28] Depth queing parameter A (offset)
  s16 zsf3{};                      // [29] Average Z scale factor (3)
  s16 zsf4{};                      // [30] Average Z scale factor (4)
  u32 flag{};                      // [31] TODO: Flag

  cpu::Cpu& m_cpu;
};

static const char* control_register_to_str(u8 reg_idx) {
  const char* gc_translate_table[] = { "R11R12", "R13R21", "R22R23", "R31R32", "R33",    "TRX",    "TRY",
                                       "TRZ",    "L11L12", "L13L21", "L22L23", "L31L32", "L33",    "RBK",
                                       "GBK",    "BBK",    "LR1LR2", "LR3LG1", "LG2LG3", "LB1LB2", "LB3",
                                       "RFC",    "GFC",    "BFC",    "OFX",    "OFY",    "H",      "DQA",
                                       "DQB",    "ZSF3",   "ZSF4",   "FLAG" };
  Expects(reg_idx < 32);
  return gc_translate_table[reg_idx];
}

static const char* data_register_to_str(u8 reg_idx) {
  const char* gd_translate_table[] = { "VXY0", "VZ0",  "VXY1", "VZ1",  "VXY2", "VZ2",  "RGB",  "OTZ",
                                       "IR0",  "IR1",  "IR2",  "IR3",  "SXY0", "SXY1", "SXY2", "SXYP",
                                       "SZ0",  "SZ1",  "SZ2",  "SZ3",  "RGB0", "RGB1", "RGB2", "RES1",
                                       "MAC0", "MAC1", "MAC2", "MAC3", "IRGB", "ORGB", "LZCS", "LZCR" };
  Expects(reg_idx < 32);
  return gd_translate_table[reg_idx];
}

}  // namespace gte
}  // namespace cpu
