#include <cpu/cpu.hpp>
#include <gui/gui.hpp>

#include <emulator/emulator.hpp>

#include <bgfx/bgfx.h>
#include <imgui.h>

#define RGBA_TO_FLOAT(r, g, b, a) ImVec4(r / 255.f, g / 255.f, b / 255.f, a / 255.f)

const ImVec4 GUI_COLOR_TTY_TEXT = RGBA_TO_FLOAT(170, 170, 170, 255);
const ImVec4 GUI_COLOR_BLACK_HALF_TRANSPARENT = RGBA_TO_FLOAT(0, 0, 0, 128);

namespace gui {

void Gui::show(const emulator::Emulator& emulator) {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("Debug")) {
      ImGui::MenuItem("TTY Output", "Ctrl+T", &m_show_tty, TTY_OUTPUT);
      ImGui::MenuItem("RAM Contents", "Ctrl+R", &m_show_ram);
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }

  show_overlay_fps();

  if (m_show_tty && TTY_OUTPUT)
    show_dialog_tty(emulator.cpu().tty_out().c_str());
  if (m_show_ram)
    show_dialog_ram(emulator.ram().data());
}

// Dialogs

void Gui::show_dialog_tty(const char* tty_text) {
  // Window style
  ImGui::SetNextWindowSize(ImVec2(470, 300), ImGuiCond_FirstUseEver);

  // Window creation
  if (!ImGui::Begin("TTY Output", &m_show_tty)) {
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

  m_show_counter++;
}

template <size_t RamSize>
void Gui::show_dialog_ram(const std::array<byte, RamSize>& ram_data) {
  // Window style
  ImGui::SetNextWindowSize(ImVec2(492, 411), ImGuiCond_FirstUseEver);

  m_ram_memeditor.DrawWindow("RAM Contents", (void*)ram_data.data(), RamSize, 0);
}

void Gui::show_overlay_fps() {
  // Don't want to deal with time, just update FPS overlay every X frames
  if (m_show_counter % 20 == 0) {
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
