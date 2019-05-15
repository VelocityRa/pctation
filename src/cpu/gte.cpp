#include <algorithm>
#include <cpu/cpu.hpp>
#include <cpu/gte.hpp>
#include <cpu/instruction.hpp>
#include <util/bit_utils.hpp>
#include <util/log.hpp>

// TODO: could eliminate Cpu friendship

namespace cpu {
namespace gte {

u32 Gte::load_data_reg(u32 src_reg) {
  switch (src_reg) {
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
    case 23: return col4_to_word(res);
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
    case 31: return lzcr;
  }

  // Unreachable
  assert(0);
  return 0;
}

u32 Gte::load_control_reg(u32 src_reg) {
  switch (src_reg) {
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
    case 58: return (s32)(s16)h;  // gte_bug: H is sign extended on read
    case 59: return (s32)dqa;
    case 60: return dqb;
    case 61: return (s32)(s16)zsf3;  // gte_bug?: sign extended
    case 62: return (s32)(s16)zsf4;  // gte_bug?: sign extended
    case 63: return flag;            // TODO
  }

  // Unreachable
  assert(0);
  return 0;
}
void Gte::store_data_reg(u32 dest_reg, u32 val) {
  switch (dest_reg) {
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
    case 23: res = col4_from_word(val); break;
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
      lzcr = bit_utils::leading_zeroes(lzcs);
      break;
    case 31: break;
  }
}
void Gte::store_control_reg(u32 dest_reg, u32 val) {
  switch (dest_reg) {
    case 0:
      rot_mat[0][0] = val;
      rot_mat[0][1] = val >> 16;
      break;
    case 1:
      rot_mat[0][2] = val;
      rot_mat[1][0] = val >> 16;
      break;
    case 2:
      rot_mat[1][1] = val;
      rot_mat[1][2] = val >> 16;
      break;
    case 3:
      rot_mat[2][0] = val;
      rot_mat[2][1] = val >> 16;
      break;
    case 4: rot_mat[2][2] = val; break;
    case 5: trans_vec.x = val; break;
    case 6: trans_vec.y = val; break;
    case 7: trans_vec.z = val; break;
    case 8:
      light_mat[0][0] = val;
      light_mat[0][1] = val >> 16;
      break;
    case 9:
      light_mat[0][2] = val;
      light_mat[1][0] = val >> 16;
      break;
    case 10:
      light_mat[1][1] = val;
      light_mat[1][2] = val >> 16;
      break;
    case 11:
      light_mat[2][0] = val;
      light_mat[2][1] = val >> 16;
      break;
    case 12: light_mat[2][2] = val; break;
    case 13: bg_col.r = val; break;
    case 14: bg_col.g = val; break;
    case 15: bg_col.b = val; break;
    case 16:
      light_col_src_mat[0][0] = val;
      light_col_src_mat[0][1] = val >> 16;
      break;
    case 17:
      light_col_src_mat[0][2] = val;
      light_col_src_mat[1][0] = val >> 16;
      break;
    case 18:
      light_col_src_mat[1][1] = val;
      light_col_src_mat[1][2] = val >> 16;
      break;
    case 19:
      light_col_src_mat[2][0] = val;
      light_col_src_mat[2][1] = val >> 16;
      break;
    case 20: light_col_src_mat[2][2] = val; break;
    case 21: far_color.r = val; break;
    case 22: far_color.g = val; break;
    case 23: far_color.b = val; break;
    case 24: screen_offset.x = val; break;
    case 25: screen_offset.y = val; break;
    case 26: h = val; break;
    case 27: dqa = val; break;
    case 28: dqb = val; break;
    case 29: zsf3 = val; break;
    case 30: zsf4 = val; break;
    case 31: flag = val; break;
  }
}

}  // namespace gte
}  // namespace cpu
