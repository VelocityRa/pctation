#include <cpu/gte.hpp>

#include <cpu/cpu.hpp>
#include <cpu/instruction.hpp>
#include <util/bit_utils.hpp>
#include <util/log.hpp>

#include <algorithm>

// Thanks to JaCzekanski (of Avocado) for some insights that make this code shorter/simpler

namespace cpu {
namespace gte {

u32 Gte::read_reg(u32 src_reg) {
  switch (src_reg) {
      // Data Registers

    case 0: return ((u16)v[0].y << 16) | (u16)v[0].x;
    case 1: return (s32)v[0].z;
    case 2: return ((u16)v[1].y << 16) | (u16)v[1].x;
    case 3: return (s32)v[1].z;
    case 4: return ((u16)v[2].y << 16) | (u16)v[2].x;
    case 5: return (s32)v[2].z;
    case 6: return col4_to_word(rgbc);
    case 7: return avg_z;
    case 8: return (s32)ir[0];
    case 9: return (s32)ir[1];
    case 10: return (s32)ir[2];
    case 11: return (s32)ir[3];
    case 12: return ((u16)s_xy[0].y << 16) | (u16)s_xy[0].x;
    case 13: return ((u16)s_xy[1].y << 16) | (u16)s_xy[1].x;
    case 14:  // fallthrough
    case 15: return ((u16)s_xy[2].y << 16) | (u16)s_xy[2].x;
    case 16: return (u16)s_z[0];
    case 17: return (u16)s_z[1];
    case 18: return (u16)s_z[2];
    case 19: return (u16)s_z[3];
    case 20: return col4_to_word(rgb_fifo[0]);
    case 21: return col4_to_word(rgb_fifo[1]);
    case 22: return col4_to_word(rgb_fifo[2]);
    case 23: return res;
    case 24: return mac[0];
    case 25: return mac[1];
    case 26: return mac[2];
    case 27: return mac[3];
    case 28:  // fallthrough
    case 29:
      rgb_conv = std::clamp(ir[1] / 0x80, 0, 0x1f);
      rgb_conv |= std::clamp(ir[2] / 0x80, 0, 0x1f) << 5;
      rgb_conv |= std::clamp(ir[3] / 0x80, 0, 0x1f) << 10;
      return rgb_conv;
    case 30: return lzcs;
    case 31:
      return lzcr;

      // Control Registers

    case 32: return ((u16)rot_mat[0][1] << 16) | (u16)rot_mat[0][0];
    case 33: return ((u16)rot_mat[1][0] << 16) | (u16)rot_mat[0][2];
    case 34: return ((u16)rot_mat[1][2] << 16) | (u16)rot_mat[1][1];
    case 35: return ((u16)rot_mat[2][1] << 16) | (u16)rot_mat[2][0];
    case 36: return (s32)rot_mat[2][2];
    case 37: return trans_vec.x;
    case 38: return trans_vec.y;
    case 39: return trans_vec.z;
    case 40: return ((u16)light_mat[0][1] << 16) | (u16)light_mat[0][0];
    case 41: return ((u16)light_mat[1][0] << 16) | (u16)light_mat[0][2];
    case 42: return ((u16)light_mat[1][2] << 16) | (u16)light_mat[1][1];
    case 43: return ((u16)light_mat[2][1] << 16) | (u16)light_mat[2][0];
    case 44: return (s32)light_mat[2][2];
    case 45: return bg_col.r;
    case 46: return bg_col.g;
    case 47: return bg_col.b;
    case 48: return ((u16)light_col_src_mat[0][1] << 16) | (u16)light_col_src_mat[0][0];
    case 49: return ((u16)light_col_src_mat[1][0] << 16) | (u16)light_col_src_mat[0][2];
    case 50: return ((u16)light_col_src_mat[1][2] << 16) | (u16)light_col_src_mat[1][1];
    case 51: return ((u16)light_col_src_mat[2][1] << 16) | (u16)light_col_src_mat[2][0];
    case 52: return (s32)light_col_src_mat[2][2];
    case 53: return far_color.r;
    case 54: return far_color.g;
    case 55: return far_color.b;
    case 56: return screen_offset.x;
    case 57: return screen_offset.y;
    case 58: return (s32)(s16)h;  // Sign extended on read
    case 59: return (s32)dqa;
    case 60: return dqb;
    case 61: return (s32)(s16)zsf3;  // Sign extended on read
    case 62: return (s32)(s16)zsf4;  // Sign extended on read
    case 63: return flag.read();
  }

  // Unreachable
  assert(0);
  return 0;
}

void Gte::write_reg(u32 dest_reg, u32 val) {
  switch (dest_reg) {
      // Data Registers

    case 0:
      v[0].x = val;
      v[0].y = val >> 16;
      break;
    case 1: v[0].z = val; break;
    case 2:
      v[1].x = val;
      v[1].y = val >> 16;
      break;
    case 3: v[1].z = val; break;
    case 4:
      v[2].x = val;
      v[2].y = val >> 16;
      break;
    case 5: v[2].z = val; break;
    case 6: rgbc = col4_from_word(val); break;
    case 7: avg_z = val; break;
    case 8: ir[0] = val; break;
    case 9: ir[1] = val; break;
    case 10: ir[2] = val; break;
    case 11: ir[3] = val; break;
    case 12:
      s_xy[0].y = val >> 16;
      s_xy[0].x = val;
      break;
    case 13:
      s_xy[1].y = val >> 16;
      s_xy[1].x = val;
      break;
    case 14:
      s_xy[2].y = val >> 16;
      s_xy[2].x = val;
      break;
    case 15:
      s_xy[0].x = s_xy[1].x;
      s_xy[0].y = s_xy[1].y;

      s_xy[1].x = s_xy[2].x;
      s_xy[1].y = s_xy[2].y;

      s_xy[2].y = val >> 16;
      s_xy[2].x = val;
      break;
    case 16: s_z[0] = val; break;
    case 17: s_z[1] = val; break;
    case 18: s_z[2] = val; break;
    case 19: s_z[3] = val; break;
    case 20: rgb_fifo[0] = col4_from_word(val); break;
    case 21: rgb_fifo[1] = col4_from_word(val); break;
    case 22: rgb_fifo[2] = col4_from_word(val); break;
    case 23: res = val; break;
    case 24: mac[0] = val; break;
    case 25: mac[1] = val; break;
    case 26: mac[2] = val; break;
    case 27: mac[3] = val; break;
    case 28:
      rgb_conv = val & 0x00007FFF;
      ir[1] = (val & 0x1F) * 0x80;
      ir[2] = ((val >> 5) & 0x1F) * 0x80;
      ir[3] = ((val >> 10) & 0x1F) * 0x80;
      break;
    case 29: break;
    case 30:
      lzcs = val;
      lzcr = (s32)bit_utils::leading_zeroes(lzcs);
      break;
    case 31:
      break;

      // Control Registers

    case 32:
      rot_mat[0][0] = val;
      rot_mat[0][1] = val >> 16;
      break;
    case 33:
      rot_mat[0][2] = val;
      rot_mat[1][0] = val >> 16;
      break;
    case 34:
      rot_mat[1][1] = val;
      rot_mat[1][2] = val >> 16;
      break;
    case 35:
      rot_mat[2][0] = val;
      rot_mat[2][1] = val >> 16;
      break;
    case 36: rot_mat[2][2] = val; break;
    case 37: trans_vec.x = val; break;
    case 38: trans_vec.y = val; break;
    case 39: trans_vec.z = val; break;
    case 40:
      light_mat[0][0] = val;
      light_mat[0][1] = val >> 16;
      break;
    case 41:
      light_mat[0][2] = val;
      light_mat[1][0] = val >> 16;
      break;
    case 42:
      light_mat[1][1] = val;
      light_mat[1][2] = val >> 16;
      break;
    case 43:
      light_mat[2][0] = val;
      light_mat[2][1] = val >> 16;
      break;
    case 44: light_mat[2][2] = val; break;
    case 45: bg_col.r = val; break;
    case 46: bg_col.g = val; break;
    case 47: bg_col.b = val; break;
    case 48:
      light_col_src_mat[0][0] = val;
      light_col_src_mat[0][1] = val >> 16;
      break;
    case 49:
      light_col_src_mat[0][2] = val;
      light_col_src_mat[1][0] = val >> 16;
      break;
    case 50:
      light_col_src_mat[1][1] = val;
      light_col_src_mat[1][2] = val >> 16;
      break;
    case 51:
      light_col_src_mat[2][0] = val;
      light_col_src_mat[2][1] = val >> 16;
      break;
    case 52: light_col_src_mat[2][2] = val; break;
    case 53: far_color.r = val; break;
    case 54: far_color.g = val; break;
    case 55: far_color.b = val; break;
    case 56: screen_offset.x = val; break;
    case 57: screen_offset.y = val; break;
    case 58: h = val; break;
    case 59: dqa = val; break;
    case 60: dqb = val; break;
    case 61: zsf3 = val; break;
    case 62: zsf4 = val; break;
    case 63: flag.word = val; break;
  }
}

s32 Gte::clamp(s32 val, s32 min, s32 max, u32 flags) {
  if (val > max) {
    flag.word |= flags;
    return max;
  }
  if (val < min) {
    flag.word |= flags;
    return min;
  }
  return val;
}

template <int BitSize>
void Gte::check_ovf_and_udf(s64 val, u32 ovf_bit, u32 udf_bit) {
  if (val >= (1LL << BitSize))
    flag.word |= ovf_bit;
  if (val < -(1LL << BitSize))
    flag.word |= udf_bit;
}

template <s64 MacIndex>
void Gte::check_mac_ovf_and_udf(s64 val) {
  if (MacIndex == 1)
    check_ovf_and_udf<43>(val, FlagRegister::MAC1_OVF_POS, FlagRegister::MAC1_OVF_NEG);
  else if (MacIndex == 2)
    check_ovf_and_udf<43>(val, FlagRegister::MAC2_OVF_POS, FlagRegister::MAC2_OVF_NEG);
  else if (MacIndex == 3)
    check_ovf_and_udf<43>(val, FlagRegister::MAC3_OVF_POS, FlagRegister::MAC3_OVF_NEG);
}

template <s64 MacIndex>
s64 Gte::check_mac_ovf_and_extend(s64 val) {
  check_mac_ovf_and_udf<MacIndex>(val);

  return bit_utils::sign_extend<43, s64>(val);
}

void Gte::mul_vec_vec(vec3_s16 v1, vec3_s16 v2, vec3_s16 tr) {
  set_mac_and_ir<1>(((s64)tr.x << 12) + v1.x * v2.x, m_lm);
  set_mac_and_ir<2>(((s64)tr.y << 12) + v1.y * v2.y, m_lm);
  set_mac_and_ir<3>(((s64)tr.z << 12) + v1.z * v2.z, m_lm);
}

template <bool Rtp>
vec3_s64 Gte::mul_mat_vec(mat3x3_s16 mat, vec3_s16 vec, vec3_s32 tr) {
  vec3_s64 product;

  product.x = check_mac_ovf_and_extend<1>(
      check_mac_ovf_and_extend<1>(check_mac_ovf_and_extend<1>(((s64)tr.x << 12) + mat[0][0] * vec.x) +
                                  mat[0][1] * vec.y) +
      mat[0][2] * vec.z);
  product.y = check_mac_ovf_and_extend<2>(
      check_mac_ovf_and_extend<2>(check_mac_ovf_and_extend<2>(((s64)tr.y << 12) + mat[1][0] * vec.x) +
                                  mat[1][1] * vec.y) +
      mat[1][2] * vec.z);
  product.z = check_mac_ovf_and_extend<3>(
      check_mac_ovf_and_extend<3>(check_mac_ovf_and_extend<3>(((s64)tr.z << 12) + mat[2][0] * vec.x) +
                                  mat[2][1] * vec.y) +
      mat[2][2] * vec.z);

  set_mac_and_ir<1>(product.x, m_lm);
  set_mac_and_ir<2>(product.y, m_lm);
  if (!Rtp)
    set_mac_and_ir<3>(product.z, m_lm);
  else
    set_mac<3>(product.z);
  return product;
}

s64 Gte::mul_mat_vec_rtp(mat3x3_s16 mat, vec3_s16 vec, vec3_s32 tr) {
  const auto product = mul_mat_vec<true>(mat, vec, tr);

  // RTP calculates the IR3 saturation flag as if the lm bit is false
  Gte::clamp((s32)(product.z >> 12), -0x8000, 0x7FFF, FlagRegister::IR3_SAT);

  // But the calculation itself respects the lm bit
  ir[3] = std::clamp(mac[3], m_lm ? 0 : -0x8000, 0x7FFF);

  return product.z;
}

template <s32 IrIndex>
void Gte::set_ir(s32 val, bool lm) {
  u32 sat_bit{};
  if (IrIndex == 1)
    sat_bit = FlagRegister::IR1_SAT;
  else if (IrIndex == 2)
    sat_bit = FlagRegister::IR2_SAT;
  else if (IrIndex == 3)
    sat_bit = FlagRegister::IR3_SAT;
  ir[IrIndex] = Gte::clamp(val, lm ? 0 : -0x8000, 0x7FFF, sat_bit);
}

template <s32 MacIndex>
s64 Gte::set_mac(s64 val) {
  check_mac_ovf_and_udf<MacIndex>(val);

  if (m_sf)
    val >>= 12;

  mac[MacIndex] = (s32)val;
  return val;
}

template <>
s64 Gte::set_mac<0>(s64 val) {
  check_ovf_and_udf<31>(val, FlagRegister::MAC0_OVF_POS, FlagRegister::MAC0_OVF_NEG);

  mac[0] = (s32)val;
  return val;
}

template <int MacIrIndex>
void Gte::set_mac_and_ir(s64 val, bool lm) {
  set_ir<MacIrIndex>((s32)set_mac<MacIrIndex>(val), lm);
}

void Gte::set_otz(s64 val) {
  avg_z = (u16)Gte::clamp((s32)(val >> 12), 0, 0xFFFF, FlagRegister::SZ3_OTZ_SAT);
}

void Gte::push_screen_xy(s32 x, s32 y) {
  s_xy[0].x = s_xy[1].x;
  s_xy[0].y = s_xy[1].y;
  s_xy[1].x = s_xy[2].x;
  s_xy[1].y = s_xy[2].y;

  s_xy[2].x = clamp(x, -0x400, 0x3FF, FlagRegister::SX2_SAT);
  s_xy[2].y = clamp(y, -0x400, 0x3FF, FlagRegister::SY2_SAT);
}

void Gte::push_screen_z(s32 z) {
  s_z[0] = s_z[1];
  s_z[1] = s_z[2];
  s_z[2] = s_z[3];

  s_z[3] = clamp(z, 0, 0xFFFF, FlagRegister::SZ3_OTZ_SAT);
}

u32 Gte::recip(u16 divisor) {
  s32 x = UNR_COUNT + UNR_TABLE[((divisor & 0x7FFF) + 0x40) >> 7];

  s32 tmp = (((s32)divisor * -x) + 0x80) >> 8;
  s32 tmp2 = ((x * (0x20000 + tmp)) + 0x80) >> 8;

  return tmp2;
}

u32 Gte::divide_unr(u32 lhs, u32 rhs) {
  if (rhs * 2 <= lhs) {
    flag.div_ovf = true;
    return DIVIDE_OVERFLOW_VAL;
  }

  const auto shift = bit_utils::leading_zeroes<u16>(rhs);
  lhs <<= shift;
  rhs <<= shift;

  const auto reciprocal = recip(rhs | 0x8000);

  const u32 result = ((u64)lhs * reciprocal + 0x8000) >> 16;

  if (result > DIVIDE_OVERFLOW_VAL) {
    flag.div_ovf = true;
    return DIVIDE_OVERFLOW_VAL;
  }

  return result;
}

void Gte::push_color(u32 r, u32 g, u32 b) {
  rgb_fifo[0] = rgb_fifo[1];
  rgb_fifo[1] = rgb_fifo[2];

  rgb_fifo[2].r = Gte::clamp(r, 0, 0xFF, FlagRegister::COLOR_R_SAT);
  rgb_fifo[2].g = Gte::clamp(g, 0, 0xFF, FlagRegister::COLOR_G_SAT);
  rgb_fifo[2].b = Gte::clamp(b, 0, 0xFF, FlagRegister::COLOR_B_SAT);
  rgb_fifo[2].a = rgbc.a;
}
void Gte::push_color() {
  push_color(mac[1] >> 4, mac[2] >> 4, mac[3] >> 4);
}

void Gte::cmd(u32 word) {
  gte::GteCommand cmd(word);

  flag.reset();
  m_sf = cmd.sf;
  m_lm = cmd.lm;

  LOG_DEBUG_GTE("COP2: {:<4}  | 0x{:08X} at 0x{:08X}", cmd.to_str(), word, m_cpu.current_pc());

  switch (cmd.opcode()) {
    case GteCommand::RTPS: cmd_rtps(); break;
    case GteCommand::NCLIP: cmd_nclip(); break;
    case GteCommand::OP: cmd_op(); break;
    case GteCommand::DPCS: cmd_dpcs(); break;
    case GteCommand::INTPL: cmd_intpl(); break;
    case GteCommand::MVMVA: cmd_mvmva(cmd.mvmva_mul_mat, cmd.mvmva_mul_vec, cmd.mvmva_trans); break;
    case GteCommand::NCDS: cmd_ncds(); break;
    case GteCommand::CDP: cmd_cdp(); break;
    case GteCommand::NCDT: cmd_ncdt(); break;
    case GteCommand::NCCS: cmd_nccs(); break;
    case GteCommand::CC: cmd_cc(); break;
    case GteCommand::NCS: cmd_ncs(); break;
    case GteCommand::NCT: cmd_nct(); break;
    case GteCommand::SQR: cmd_sqr(); break;
    case GteCommand::DCPL: cmd_dcpl(); break;
    case GteCommand::DPCT: cmd_dpct(); break;
    case GteCommand::AVSZ3: cmd_avsz3(); break;
    case GteCommand::AVSZ4: cmd_avsz4(); break;
    case GteCommand::RTPT: cmd_rtpt(); break;
    case GteCommand::GPF: cmd_gpf(); break;
    case GteCommand::GPL: cmd_gpl(); break;
    case GteCommand::NCCT: cmd_ncct(); break;
  }
}

void Gte::cmd_rtps(u8 vec_idx, bool set_mac0) {
  s64 mac3 = mul_mat_vec_rtp(rot_mat, v[vec_idx], trans_vec);

  push_screen_z((s32)(mac3 >> 12));
  s64 h_s3z = divide_unr(h, s_z[3]);

  s32 x = (s32)(set_mac<0>((s64)(h_s3z * ir[1]) + screen_offset[0]) >> 16);
  s32 y = (s32)(set_mac<0>(h_s3z * ir[2] + screen_offset[1]) >> 16);
  push_screen_xy(x, y);

  if (set_mac0) {
    s64 mac0 = set_mac<0>(h_s3z * dqa + dqb);
    ir[0] = Gte::clamp((s32)(mac0 >> 12), 0, 0x1000, FlagRegister::IR0_SAT);
  }
}

void Gte::cmd_nclip() {
  set_mac<0>((s64)s_xy[0].x * s_xy[1].y + s_xy[1].x * s_xy[2].y + s_xy[2].x * s_xy[0].y -
             s_xy[0].x * s_xy[2].y - s_xy[1].x * s_xy[0].y - s_xy[2].x * s_xy[1].y);
}

void Gte::cmd_op() {}

void Gte::cmd_dpcs(bool use_rgb0) {
  s16 r = use_rgb0 ? rgb_fifo[0].r << 4 : rgbc_r();
  s16 g = use_rgb0 ? rgb_fifo[0].g << 4 : rgbc_g();
  s16 b = use_rgb0 ? rgb_fifo[0].b << 4 : rgbc_b();

  set_mac_and_ir<1>(((s64)far_color.r << 12) - (r << 12));
  set_mac_and_ir<2>(((s64)far_color.r << 12) - (g << 12));
  set_mac_and_ir<3>(((s64)far_color.b << 12) - (b << 12));

  mul_vec_vec(ir0_to_vec(), ir_to_vec(), vec3_s16(r, g, b));
  push_color();
}

void Gte::cmd_intpl() {}

void Gte::cmd_mvmva(u32 mul_mat_idx, u32 mul_vec_idx, u32 tr_vec_idx) {
  mat3x3_s16 mul_mat;
  if (mul_mat_idx == 0)
    mul_mat = rot_mat;
  else if (mul_mat_idx == 0)
    mul_mat = rot_mat;
  else if (mul_mat_idx == 0)
    mul_mat = rot_mat;
  else {
    mul_mat = {};
    LOG_ERROR_GTE("Buggy matrix selected");
  }

  vec3_s16 mul_vec;
  if (mul_vec_idx == 0)
    mul_vec = v[0];
  else if (mul_vec_idx == 1)
    mul_vec = v[1];
  else if (mul_vec_idx == 2)
    mul_vec = v[2];
  else {
    mul_vec.x = ir[1];
    mul_vec.y = ir[2];
    mul_vec.z = ir[3];
  }
  vec3_s32 tr_vec;
  if (tr_vec_idx == 0)
    tr_vec = trans_vec;
  else if (tr_vec_idx == 1)
    tr_vec = bg_col;
  else if (tr_vec_idx == 2) {
    tr_vec = {};
    LOG_ERROR_GTE("Buggy FarColor vector operation");
  } else
    tr_vec = {};

  mul_mat_vec(mul_mat, mul_vec, tr_vec);
}

void Gte::cmd_ncds(u8 vec_idx) {
  mul_mat_vec(light_mat, v[vec_idx]);

  mul_mat_vec(light_col_src_mat, ir_to_vec(), bg_col);

  const auto ir_vec_tmp = ir_to_vec();

  set_mac_and_ir<1>(((s64)far_color.r << 12) - (rgbc_r() * ir[1]));
  set_mac_and_ir<2>(((s64)far_color.g << 12) - (rgbc_g() * ir[2]));
  set_mac_and_ir<3>(((s64)far_color.b << 12) - (rgbc_b() * ir[3]));

  set_mac_and_ir<1>((rgbc_r() * ir_vec_tmp.x) + ir[0] * ir[1], m_lm);
  set_mac_and_ir<2>((rgbc_g() * ir_vec_tmp.y) + ir[0] * ir[2], m_lm);
  set_mac_and_ir<3>((rgbc_b() * ir_vec_tmp.z) + ir[0] * ir[3], m_lm);

  push_color();
}

void Gte::cmd_cdp() {}

void Gte::cmd_ncdt() {
  cmd_ncds(0);
  cmd_ncds(1);
  cmd_ncds(2);
}

void Gte::cmd_nccs(u8 vec_idx) {
  mul_mat_vec(light_mat, v[vec_idx]);
  mul_mat_vec(light_col_src_mat, ir_to_vec(), bg_col);
  mul_vec_vec(rgbc_to_vec(), ir_to_vec());
  push_color();
}

void Gte::cmd_cc() {
  mul_mat_vec(light_col_src_mat, ir_to_vec(), bg_col);
  mul_vec_vec(rgbc_to_vec(), ir_to_vec());
  push_color();
}

void Gte::cmd_ncs(u8 vec_idx) {
  mul_mat_vec(light_mat, v[vec_idx]);
  mul_mat_vec(light_col_src_mat, ir_to_vec(), bg_col);
  push_color();
}

void Gte::cmd_nct() {
  cmd_ncs(0);
  cmd_ncs(1);
  cmd_ncs(2);
}

void Gte::cmd_sqr() {
  mul_vec_vec(ir_to_vec(), ir_to_vec());
}

void Gte::cmd_dcpl() {}

void Gte::cmd_dpct() {
  cmd_dpcs(true);
  cmd_dpcs(true);
  cmd_dpcs(true);
}

void Gte::cmd_avsz3() {
  set_otz(set_mac<0>((s64)zsf3 * (s_z[1] + s_z[2] + s_z[3])));
}

void Gte::cmd_avsz4() {
  set_otz(set_mac<0>((s64)zsf4 * (s_z[0] + s_z[1] + s_z[2] + s_z[3])));
}

void Gte::cmd_rtpt() {
  cmd_rtps(0, false);
  cmd_rtps(1, false);
  cmd_rtps(2);
}

void Gte::cmd_gpf() {
  mul_vec_vec(ir0_to_vec(), ir_to_vec());
  push_color();
}

void Gte::cmd_gpl() {
  set_mac_and_ir<1>(((s64)mac[1] << (m_sf * 12)) + ir[0] * ir[1], m_lm);
  set_mac_and_ir<2>(((s64)mac[2] << (m_sf * 12)) + ir[0] * ir[2], m_lm);
  set_mac_and_ir<3>(((s64)mac[3] << (m_sf * 12)) + ir[0] * ir[3], m_lm);
  push_color();
}

void Gte::cmd_ncct() {
  cmd_nccs(0);
  cmd_nccs(1);
  cmd_nccs(2);
}

}  // namespace gte
}  // namespace cpu
