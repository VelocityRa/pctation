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

  u32 read_reg(u32 src_reg);
  void write_reg(u32 dest_reg, u32 val);
  void cmd(u32 word);

 private:
  union FlagRegister {
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

  // Commands
  void cmd_rtps();
  void cmd_nclip();
  void cmd_op();
  void cmd_dpcs();
  void cmd_intpl();
  void cmd_mvmva();
  void cmd_ncds();
  void cmd_cdp();
  void cmd_ncdt();
  void cmd_nccs();
  void cmd_cc();
  void cmd_ncs();
  void cmd_nct();
  void cmd_sqr();
  void cmd_dcpl();
  void cmd_dpct();
  void cmd_avsz3();
  void cmd_avsz4();
  void cmd_rtpt();
  void cmd_gpf();
  void cmd_gpl();
  void cmd_ncct();

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
