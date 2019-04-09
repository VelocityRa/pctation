#include <gui/gui.hpp>

#include <cpu/cpu.hpp>
#include <gpu/gpu.hpp>

#include <emulator/emulator.hpp>

#include <bgfx/bgfx.h>
#include <imgui.h>

#define RGBA_TO_FLOAT(r, g, b, a) ImVec4(r / 255.f, g / 255.f, b / 255.f, a / 255.f)

const auto GUI_COLOR_TTY_TEXT = RGBA_TO_FLOAT(170, 170, 170, 255);
const auto GUI_COLOR_BLACK_HALF_TRANSPARENT = RGBA_TO_FLOAT(0, 0, 0, 128);
const auto GUI_TABLE_COLUMN_TITLES_COL = ImColor(255, 255, 255);
const auto GUI_TABLE_COLUMN_ROWS_NAME_COL = IM_COL32(200, 200, 200, 255);
const auto GUI_TABLE_COLUMN_ROWS_OTHER_COL = IM_COL32(150, 150, 150, 255);

namespace gui {

void Gui::draw(const emulator::Emulator& emulator) {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("Debug")) {
      ImGui::MenuItem("TTY Output", "Ctrl+T", &m_draw_tty, TTY_OUTPUT);
      ImGui::MenuItem("RAM Contents", "Ctrl+R", &m_draw_ram);
      ImGui::MenuItem("GPU Registers", "Ctrl+G", &m_draw_gpu_registers);
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }

  draw_overlay_fps();

  if (m_draw_tty && TTY_OUTPUT)
    draw_dialog_tty(emulator.cpu().tty_out().c_str());
  if (m_draw_ram)
    draw_dialog_ram(emulator.ram().data());
  if (m_draw_gpu_registers)
    draw_gpu_registers(emulator.gpu());

  m_draw_counter++;
}

// Dialogs

void Gui::draw_dialog_tty(const char* tty_text) {
  // Window style
  ImGui::SetNextWindowSize(ImVec2(470, 300), ImGuiCond_FirstUseEver);

  // Window creation
  if (!ImGui::Begin("TTY Output", &m_draw_tty)) {
    ImGui::End();
    return;
  }

  // Options above log text
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
  ImGui::Checkbox("Auto-scroll", &m_tty_autoscroll);
  ImGui::PopStyleVar();
  ImGui::Separator();

  // TTY text
  ImGui::BeginChild("tty text", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
  ImGui::TextColored(GUI_COLOR_TTY_TEXT, tty_text);

  if (m_tty_autoscroll)
    ImGui::SetScrollHere(1.0f);
  ImGui::EndChild();

  ImGui::End();
}

template <size_t RamSize>
void Gui::draw_dialog_ram(const std::array<byte, RamSize>& ram_data) {
  // Window style
  ImGui::SetNextWindowSize(ImVec2(500, 411), ImGuiCond_FirstUseEver);

  m_ram_memeditor.DrawWindow("RAM Contents", (void*)ram_data.data(), RamSize, 0);
}

struct RegisterTableEntry {
  const char* bits;
  const char* description;
  std::string value;
};
using RegisterTableEntries = std::vector<RegisterTableEntry>;

const float REG_TABLE_WIDTH = 300.f;
const u32 REG_TABLE_COUNT = 3;

static void draw_register_table(const char* title, const RegisterTableEntries rows) {
  // TODO: eliminate
  static float g_char_width = ImGui::CalcTextSize("_").x;

  const char* header_titles[] = { "Bits", "Description", "Value" };
  const size_t column_count = IM_ARRAYSIZE(header_titles);
  float column_max_chars[] = { 8, 22, 8 };

  ImGui::BulletText(title);

  ImGui::SetNextWindowSize(ImVec2(0, 0));
  ImGui::SetNextWindowContentSize(ImVec2(REG_TABLE_WIDTH, 0.0f));

  if (ImGui::BeginChild(title, ImVec2(0, 0), true)) {
    // Draw headers
    ImGui::Columns(column_count, title);
    for (auto i = 0; i < column_count; ++i) {
      ImGui::SetColumnWidth(i, column_max_chars[i] * g_char_width + 25);
    }

    ImGui::Separator();
    for (auto i = 0; i < column_count; ++i) {
      ImGui::TextColored(GUI_TABLE_COLUMN_TITLES_COL, "%s", header_titles[i]);
      ImGui::NextColumn();
    }
    ImGui::Separator();

    char text[50];

    ImGui::PushStyleColor(ImGuiCol_Text, GUI_TABLE_COLUMN_ROWS_NAME_COL);

    for (auto row : rows) {
      snprintf(text, sizeof text, "%s", row.bits);
      ImGui::Selectable(text);
      ImGui::NextColumn();

      snprintf(text, sizeof text, "%s", row.description);
      ImGui::Selectable(text);
      ImGui::NextColumn();

      ImGui::PushStyleColor(ImGuiCol_Text, GUI_TABLE_COLUMN_ROWS_OTHER_COL);
      snprintf(text, sizeof text, "%s", row.value.c_str());
      ImGui::Selectable(text);
      ImGui::NextColumn();
      ImGui::PopStyleColor();
    }

    ImGui::PopStyleColor();
  }
  ImGui::EndChild();
}

void Gui::draw_gpu_registers(const gpu::Gpu& gpu) {
  ImGui::SetNextWindowSize(ImVec2(REG_TABLE_WIDTH * REG_TABLE_COUNT, 0), ImGuiCond_FirstUseEver);

  if (!ImGui::Begin("GPU Registers", &m_draw_gpu_registers, 0)) {
    ImGui::End();
    return;
  }

  ImGui::SetNextWindowContentSize(ImVec2(REG_TABLE_WIDTH * REG_TABLE_COUNT, 0.0f));
  ImGui::BeginChild("##ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
  ImGui::Columns(REG_TABLE_COUNT, "reg table");

  // Generic value to string tables
  const char* off_on_str[] = { "Off", "On" };
  const char* on_off_str[] = { "On", "Off" };

  // Draw GPUSTAT fields

  // For brevity
  const auto gs = gpu.gpustat();

  // GPUSTAT fields to string tables
  const char* semi_transparency_str[] = { "B/2+F/2", "B+F", "B-F", "B+F/4" };
  const char* tex_page_colors_str[] = { "4bit", "8bit", "15bit", "Reserv." };
  const char* video_mode_str[] = { "NTSC", "PAL" };
  const char* disp_color_depth_str[] = { "15bit", "24bit" };
  const char* dma_direction_str[] = { "Off", "???", "CPUtoGP0", "VRMtoCPU" };
  const char* interlace_type_str[] = { "Even", "Odd" };

  const RegisterTableEntries gpustat_rows = {
    { "0-3", "Texture Page X Base", fmt::format("{}", gs.tex_page_x_base * 64) },
    { "4", "Texture Page Y Base", fmt::format("{}", gs.tex_page_y_base * 256) },
    { "5-6", "Semi-transparency", semi_transparency_str[gs.semi_transparency] },
    { "7-8", "Texture Page Colors", tex_page_colors_str[gs.tex_page_colors] },
    { "9", "Dither 24b to 15b", off_on_str[gs.dither_en] },
    { "10", "Drawing to display", off_on_str[gs.drawing_to_disp_en] },
    { "11", "Set mask bit on draw", off_on_str[gs.force_set_mask_bit] },
    { "12", "Draw Pixels", on_off_str[gs.preserve_masked_bits] },
    { "13", "Interlace Field", fmt::format("{}", gs.interlace_field) },
    { "14", "Reverseflag", fmt::format("{}", gs.reverse_flag) },
    { "15", "Texture Disable", fmt::format("{}", gs.tex_disable) },
    { "16", "Horizontal Res. 2", fmt::format("{}", gs.horizontal_res_2) },
    { "17-18", "Horizontal Res. 1", fmt::format("{}", gs.horizontal_res_1) },
    { "19", "Vertical Resolution", fmt::format("{}", gs.vertical_res) },
    { "20", "Video mode", video_mode_str[gs.video_mode] },
    { "21", "Display Color Depth", disp_color_depth_str[gs.disk_color_depth] },
    { "22", "Vertical Interlace", fmt::format("{}", gs.vertical_interlace) },
    { "23", "Display Enabled", on_off_str[gs.disp_disabled] },
    { "24", "Interrupt Request", fmt::format("{}", gs.interrupt) },
    { "25", "DMA / Data Request", fmt::format("{}", gs.dma_data_req) },
    { "26", "Ready to recv cmd", off_on_str[gs.ready_to_recv_cmd] },
    { "27", "Ready to send VRAM", off_on_str[gs.ready_to_send_vram_to_cpu] },
    { "28", "Ready to recv DMA", off_on_str[gs.ready_to_recv_dma_block] },
    { "29-30", "DMA Direction", dma_direction_str[gs.dma_direction_] },
    { "31", "Interlace type", interlace_type_str[gs.interlace_drawing_mode] },
  };

  draw_register_table("GPUSTAT", gpustat_rows);

  // Draw GP0 fields
  const RegisterTableEntries gp0_rows = {
    { "E1h.12", "Text. rect. X Flip", off_on_str[gpu.m_draw_mode.rect_textured_x_flip] },
    { "E1h.13", "Text. rect. Y Flip", off_on_str[gpu.m_draw_mode.rect_textured_y_flip] },
    { "E2h.0-4", "Texture window Mask X", fmt::format("{}", gpu.m_tex_window.tex_window_mask_x * 8) },
    { "E2h.5-9", "Texture window Mask Y", fmt::format("{}", gpu.m_tex_window.tex_window_mask_y * 8) },
    { "E2h.10-14", "Text. wind. offset X", fmt::format("{}", gpu.m_tex_window.tex_window_off_x * 8) },
    { "E2h.15-19", "Text. wind. offset Y", fmt::format("{}", gpu.m_tex_window.tex_window_off_y * 8) },
    { "E3h.0-9", "Draw Area top left X", fmt::format("{}", gpu.m_drawing_area_top_left.x_coord) },
    { "E3h.10-18", "Draw Area top left Y", fmt::format("{}", gpu.m_drawing_area_top_left.y_coord) },
    { "E4h.0-9", "Draw Area bott. right X", fmt::format("{}", gpu.m_drawing_area_bottom_right.x_coord) },
    { "E4h.10-18", "Draw Area bott. right Y",
      fmt::format("{}", gpu.m_drawing_area_bottom_right.y_coord) },
    { "E5h.0-10", "Drawing offset X", fmt::format("{}", gpu.m_drawing_offset.x_off) },
    { "E5h.10-19", "Drawing offset Y", fmt::format("{}", gpu.m_drawing_offset.y_off) },
  };

  ImGui::NextColumn();

  draw_register_table("GP0", gp0_rows);

  // Draw GP1 fields
  const RegisterTableEntries gp1_rows = {
    { "05h.0-10", "Display area X", fmt::format("{}", gpu.m_display_area.x_coord) },
    { "05h.10-18", "Display area Y", fmt::format("{}", gpu.m_display_area.y_coord) },
    { "06h.0-11", "Horiz. disp. range X1", fmt::format("{}", gpu.m_hdisplay_range.x1) },
    { "06h.12-23", "Horiz. disp. range X2", fmt::format("{}", gpu.m_hdisplay_range.x2) },
    { "07h.0-9", "Vert. disp. range Y1", fmt::format("{}", gpu.m_vdisplay_range.y1) },
    { "07h.10-19", "Vert. disp. range Y2", fmt::format("{}", gpu.m_vdisplay_range.y2) },
  };

  ImGui::NextColumn();

  draw_register_table("GP1", gp1_rows);

  ImGui::Columns(1);
  ImGui::EndChild();

  ImGui::End();
}

void Gui::draw_overlay_fps() {
  // Don't want to deal with time, just update FPS overlay every X frames
  if (m_draw_counter % 20 == 0) {
    const bgfx::Stats* stats = bgfx::getStats();
    const double toMsCpu = 1000.0 / stats->cpuTimerFreq;
    const double frameMs = double(stats->cpuTimeFrame) * toMsCpu;
    m_fps_overlay_str = fmt::format("FPS: {:0.2f}", 1000.0 / frameMs);
  }

  const auto imgui_flags_overlay = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs;

  ImGui::SetNextWindowPos(ImVec2{ 8.f, 25.f });
  ImGui::SetNextWindowSize(ImVec2{ 88.f, 0.f });
  ImGui::PushStyleColor(ImGuiCol_WindowBg, GUI_COLOR_BLACK_HALF_TRANSPARENT);

  ImGui::Begin("FPS", nullptr, imgui_flags_overlay);
  ImGui::TextUnformatted(m_fps_overlay_str.c_str());
  ImGui::End();

  ImGui::PopStyleColor();
}

}  // namespace gui
