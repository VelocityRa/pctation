#pragma once

#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>
#include <gsl-lite.hpp>
#include <util/types.hpp>

#include <array>

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
using vec3_s64 = glm::vec<3, s64, glm::defaultp>;

using mat3x3_s16 = glm::mat<3, 3, s16, glm::defaultp>;

using col4_u8 = glm::vec<4, u8, glm::defaultp>;
using col3_s32 = glm::vec<3, s32, glm::defaultp>;

constexpr auto DIVIDE_OVERFLOW_VAL = 0x1FFFF;
constexpr auto UNR_COUNT = 0x101;

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

constexpr std::array<u8, UNR_COUNT> generate_unr_table() {
  std::array<u8, UNR_COUNT> table = { { 0 } };
  for (auto i = 0; i < (s32)table.size(); i++) {
    table[i] = std::max(0, (0x40000 / (i + 0x100) + 1) / 2 - UNR_COUNT);
  }
  return table;
}

class Gte {
 public:
  explicit Gte(cpu::Cpu& cpu) : UNR_TABLE(generate_unr_table()), m_cpu(cpu) {}

  u32 read_reg(u32 src_reg);
  void write_reg(u32 dest_reg, u32 val);
  void cmd(u32 word);

 private:
  union FlagRegister {
    enum {
      IR0_SAT = 1 << 12,
      SY2_SAT = 1 << 13,
      SX2_SAT = 1 << 14,
      MAC0_OVF_NEG = 1 << 15,
      MAC0_OVF_POS = 1 << 16,
      DIVIDE_OVF = 1 << 17,
      SZ3_OTZ_SAT = 1 << 18,
      COLOR_B_SAT = 1 << 19,
      COLOR_G_SAT = 1 << 20,
      COLOR_R_SAT = 1 << 21,
      IR3_SAT = 1 << 22,
      IR2_SAT = 1 << 23,
      IR1_SAT = 1 << 24,
      MAC3_OVF_NEG = 1 << 25,
      MAC2_OVF_NEG = 1 << 26,
      MAC1_OVF_NEG = 1 << 27,
      MAC3_OVF_POS = 1 << 28,
      MAC2_OVF_POS = 1 << 29,
      MAC1_OVF_POS = 1 << 30,
      FLAG = 1 << 31
    };

    u32 word{};

    struct {
      u32 : 12;              // [0-11] Not used (always zero) (Read only)
      u32 ir0_sat : 1;       // [12]   IR0 saturated to +0000h..+1000h
      u32 sy2_sat : 1;       // [13]   SY2 saturated to -0400h..+03FFh
      u32 sx2_sat : 1;       // [14]   SX2 saturated to -0400h..+03FFh
      u32 mac0_ovf_neg : 1;  // [15]   MAC0 Result larger than 31 bits and negative
      u32 mac0_ovf_pos : 1;  // [16]   MAC0 Result larger than 31 bits and positive
      u32 div_ovf : 1;       // [17]   Divide overflow. RTPS/RTPT division result saturated to max=1FFFFh
      u32 sz3_otz_sat : 1;   // [18]   SZ3 or OTZ saturated to +0000h..+FFFFh
      u32 col_b_sat : 1;     // [19]   Color-FIFO-B saturated to +00h..+FFh
      u32 col_g_sat : 1;     // [20]   Color-FIFO-G saturated to +00h..+FFh
      u32 col_r_sat : 1;     // [21]   Color-FIFO-R saturated to +00h..+FFh
      u32 ir3_sat : 1;       // [22]   IR3 saturated to +0000h..+7FFFh (lm=1) or to -8000h..+7FFFh (lm=0)
      u32 ir2_sat : 1;       // [23]   IR2 saturated to +0000h..+7FFFh (lm=1) or to -8000h..+7FFFh (lm=0)
      u32 ir1_sat : 1;       // [24]   IR1 saturated to +0000h..+7FFFh (lm=1) or to -8000h..+7FFFh (lm=0)
      u32 mac3_ovf_neg : 1;  // [25]   MAC3 Result larger than 43 bits and negative
      u32 mac2_ovf_neg : 1;  // [26]   MAC2 Result larger than 43 bits and negative
      u32 mac1_ovf_neg : 1;  // [27]   MAC1 Result larger than 43 bits and negative
      u32 mac3_ovf_pos : 1;  // [28]   MAC3 Result larger than 43 bits and positive
      u32 mac2_ovf_pos : 1;  // [29]   MAC2 Result larger than 43 bits and positive
      u32 mac1_ovf_pos : 1;  // [30]   MAC1 Result larger than 43 bits and positive
      u32 flag : 1;          // [31]   Error Flag (Bit30..23, and 18..13 ORed together) (Read only)
    };

    void reset() { word = 0; }
    u32 read() {
      flag = ((word & 0x7F87E000) != 0);  // Calculate flag
      word &= 0xFFFFF000;                 // Mask out zero bits
      return word;
    }
  };

  //
  // Command helpers
  //

  s32 clamp(s32 val, s32 min, s32 max, u32 flags = 0);

  // Check for overflow/underflow and set appropriate bits if any
  template <s32 BitSize>
  void check_ovf_and_udf(s64 val, u32 ovf_bit, u32 udf_bit);
  template <s64 MacIndex>
  void check_mac_ovf_and_udf(s64 val);
  template <s64 MacIndex>
  s64 check_mac_ovf_and_extend(s64 val);

  void mul_vec_vec(vec3_s16 v1, vec3_s16 v2, vec3_s16 tr = {});
  template <bool Rtp = false>
  vec3_s64 mul_mat_vec(mat3x3_s16 mat, vec3_s16 vec, vec3_s32 tr = {});
  s64 mul_mat_vec_rtp(mat3x3_s16 mat, vec3_s16 vec, vec3_s32 tr);

  template <s32 IrIndex>
  void set_ir(s32 val, bool lm = false);
  template <s32 MacIndex>
  s64 set_mac(s64 val);
  template <s32 MacIrIndex>
  void set_mac_and_ir(s64 val, bool lm = false);
  void set_otz(s64 val);

  void push_screen_xy(s32 x, s32 y);
  void push_screen_z(s32 z);
  void push_color(u32 r, u32 g, u32 b);
  void push_color();

  u32 recip(u16 divisor);
  u32 divide_unr(u32 lhs, u32 rhs);

  s16 rgbc_r() const { return rgbc.r << 4; }
  s16 rgbc_g() const { return rgbc.g << 4; }
  s16 rgbc_b() const { return rgbc.b << 4; }
  vec3_s16 ir_to_vec() const { return { ir[1], ir[2], ir[3] }; }
  vec3_s16 ir0_to_vec() const { return { ir[0], ir[0], ir[0] }; }
  vec3_s16 rgbc_to_vec() const { return { rgbc_r(), rgbc_g(), rgbc_b() }; }

  bool m_sf{};
  bool m_lm{};
  const std::array<u8, UNR_COUNT> UNR_TABLE;

  //
  // Commands
  //

  void cmd_rtps(u8 vec_n = 0,
                bool set_mac0 = true);   // Coordinate transformation and perspective transformation
  void cmd_nclip();                      // Normal clipping
  void cmd_op();                         // Outer product
  void cmd_dpcs(bool use_rgb0 = false);  // Depth queuing
  void cmd_intpl();                      // Interpolation
  void cmd_mvmva(u32 mul_mat, u32 mul_vec, u32 trans_vec);  // Matrix and vector multiplication
  void cmd_ncds(u8 vec_idx = 0);                            // Light source calculation
  void cmd_cdp();                                           // Light source calculation
  void cmd_ncdt();                                          // Light source calculation
  void cmd_nccs(u8 vec_idx = 0);                            // Light source calculation
  void cmd_cc();                                            // Light source calculation
  void cmd_ncs(u8 vec_idx = 0);                             // Light source calculation
  void cmd_nct();                                           // Light source calculation
  void cmd_sqr();                                           // Vector squaring
  void cmd_dcpl();                                          // Depth queuing
  void cmd_dpct();                                          // Depth queuing
  void cmd_avsz3();                                         // Calculate average of 3 Z values
  void cmd_avsz4();                                         // Calculate average of 4 Z values
  void cmd_rtpt();  // Coordinate transformation and perspective transformation
  void cmd_gpf();   // General purpose interpolation
  void cmd_gpl();   // General purpose interpolation
  void cmd_ncct();  // Light source calculation

 private:
  //
  // Registers
  //

  // Data Registers
  vec3_s16 v[3]{};        // [0-5] Vector 0 - 3
  col4_u8 rgbc{};         // [6] Color/code values
  u16 avg_z{};            // [7] Average Z values (for Ordering Table)
  s16 ir[4]{};            // [8-11] 16bit Accumulator (0: Interpolate, 1-3: Vector)
  vec2_s16 s_xy[4]{};     // [12-15] Screen XY-coordinate FIFO  (3 stages)
  u16 s_z[4]{};           // [16-19] Screen Z-coordinate FIFO   (4 stages)
  col4_u8 rgb_fifo[3]{};  // [20-22] Color CRGB-code/color FIFO (3 stages)
  u32 res{};              // [23] Prohibited (?)
  s32 mac[4]{};           // [24-27] 32bit Maths Accumulators (0: Value, 1-3: Vector)
  u16 rgb_conv{};         // [28-29] Convert RGB Color (48bit vs 15bit)
  s32 lzcs{};             // Count Leading-Zeroes/Ones (sign bits)
  s32 lzcr{};             // Count Leading-Zeroes/Ones (sign bits)

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
  s32 dqb{};                       // [28] Depth queing parameter A (offset)
  s16 zsf3{};                      // [29] Average Z scale factor (3)
  s16 zsf4{};                      // [30] Average Z scale factor (4)
  FlagRegister flag{};             // [31]Flag

  cpu::Cpu& m_cpu;
};

union GteCommand {
  enum Opcode {
    RTPS = 0x01,
    NCLIP = 0x06,
    OP = 0x0C,
    DPCS = 0x10,
    INTPL = 0x11,
    MVMVA = 0x12,
    NCDS = 0x13,
    CDP = 0x14,
    NCDT = 0x16,
    NCCS = 0x1B,
    CC = 0x1C,
    NCS = 0x1E,
    NCT = 0x20,
    SQR = 0x28,
    DCPL = 0x29,
    DPCT = 0x2A,
    AVSZ3 = 0x2D,
    AVSZ4 = 0x2E,
    RTPT = 0x30,
    GPF = 0x3D,
    GPL = 0x3E,
    NCCT = 0x3F,
  };

  explicit GteCommand(u32 word) : word(word) {}

  u32 word;
  struct {
    u32 _opcode : 6;  // [0-5]    Real GTE Command Number (00h..3Fh) (used by hardware)
    u32 : 4;          // [6-9]    Always zero                        (ignored by hardware)
    u32 lm : 1;       // [10]     lm - Saturate IR1,IR2,IR3 result (0=To -8000h..+7FFFh, 1=To 0..+7FFFh)
    u32 : 2;          // [11-12]  Always zero                        (ignored by hardware)
    u32 mvmva_trans : 2;    // [13-14]  MVMVA Translation Vector (0=TR, 1=BK, 2=FC/Bugged, 3=None)
    u32 mvmva_mul_vec : 2;  // [15-16]  MVMVA Multiply Vector    (0=V0, 1=V1, 2=V2, 3=IR/long)
    u32 mvmva_mul_mat : 2;  // [17-18]  MVMVA Multiply Matrix    (0=Rotation. 1=Light, 2=Color,
                            // 3=Reserved)
    u32 sf : 1;       // [19]     sf - Shift Fraction in IR registers (0=No fraction, 1=12bit fraction)
    u32 op_fake : 5;  // [20-24]  Fake GTE Command Number (00h..1Fh) (ignored by hardware)
    u32 : 7;          // [31-25]  Must be 0100101b for "COP2 imm25" instructions
  };

  Opcode opcode() const { return static_cast<Opcode>(_opcode); }

  const char* to_str() const {
    switch (_opcode) {
      case RTPS: return "RTPS";
      case NCLIP: return "NCLIP";
      case OP: return "OP";
      case DPCS: return "DPCS";
      case INTPL: return "INTPL";
      case MVMVA: return "MVMVA";
      case NCDS: return "NCDS";
      case CDP: return "CDP";
      case NCDT: return "NCDT";
      case NCCS: return "NCCS";
      case CC: return "CC";
      case NCS: return "NCS";
      case NCT: return "NCT";
      case SQR: return "SQR";
      case DCPL: return "DCPL";
      case DPCT: return "DPCT";
      case AVSZ3: return "AVSZ3";
      case AVSZ4: return "AVSZ4";
      case RTPT: return "RTPT";
      case GPF: return "GPF";
      case GPL: return "GPL";
      case NCCT: return "NCCT";
      default: return "<invalid>";
    }
  }
};

static const char* reg_to_str(u8 reg_idx) {
  const char* reg_translate_table[] = {
    "VXY0",   "VZ0",    "VXY1",   "VZ1",    "VXY2",   "VZ2",    "RGB",  "OTZ",  "IR0",    "IR1",
    "IR2",    "IR3",    "SXY0",   "SXY1",   "SXY2",   "SXYP",   "SZ0",  "SZ1",  "SZ2",    "SZ3",
    "RGB0",   "RGB1",   "RGB2",   "RES1",   "MAC0",   "MAC1",   "MAC2", "MAC3", "IRGB",   "ORGB",
    "LZCS",   "LZCR",   "R11R12", "R13R21", "R22R23", "R31R32", "R33",  "TRX",  "TRY",    "TRZ",
    "L11L12", "L13L21", "L22L23", "L31L32", "L33",    "RBK",    "GBK",  "BBK",  "LR1LR2", "LR3LG1",
    "LG2LG3", "LB1LB2", "LB3",    "RFC",    "GFC",    "BFC",    "OFX",  "OFY",  "H",      "DQA",
    "DQB",    "ZSF3",   "ZSF4",   "FLAG"
  };
  Expects(reg_idx < 64);
  return reg_translate_table[reg_idx];
}

}  // namespace gte
}  // namespace cpu
