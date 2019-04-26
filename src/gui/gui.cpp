#include <gui/gui.hpp>

#include <cpu/cpu.hpp>
#include <gpu/gpu.hpp>
#include <util/log.hpp>

#include <emulator/emulator.hpp>

#include <SDL2/SDL.h>
#pragma warning(disable : 4251)  // hide some glbinding warnings
#include <glbinding-aux/types_to_string.h>
#include <glbinding/Binding.h>
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
#pragma warning(default : 4251)
#include <imgui.h>
#include <imgui_sdl2_gl3_backend/imgui_impl_opengl3.h>
#include <imgui_sdl2_gl3_backend/imgui_impl_sdl.h>

#include <cassert>
#include <exception>
#include <sstream>

#define RGBA_TO_FLOAT(r, g, b, a) ImVec4(r / 255.f, g / 255.f, b / 255.f, a / 255.f)

using namespace gl;

const auto GUI_CLEAR_COLOR = RGBA_TO_FLOAT(133, 20, 75, 255);
const auto GUI_COLOR_TTY_TEXT = RGBA_TO_FLOAT(170, 170, 170, 255);
const auto GUI_COLOR_BLACK_HALF_TRANSPARENT = RGBA_TO_FLOAT(0, 0, 0, 128);
const auto GUI_TABLE_COLUMN_TITLES_COL = ImColor(255, 255, 255);
const auto GUI_TABLE_COLUMN_ROWS_NAME_COL = IM_COL32(200, 200, 200, 255);
const auto GUI_TABLE_COLUMN_ROWS_OTHER_COL = IM_COL32(150, 150, 150, 255);

#define SDL_ERROR(msg)                                                      \
  do {                                                                      \
    LOG_ERROR(msg##": {}", SDL_GetError());                                 \
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", msg, m_window); \
    throw std::runtime_error(#msg);                                         \
  } while (0);

// Use a discrete GPU if available
extern "C" {
// http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
__declspec(dllexport) uint32_t NvOptimusEnablement = 1;

// https://gpuopen.com/amdpowerxpressrequesthighperformance/
__declspec(dllexport) uint32_t AmdPowerXpressRequestHighPerformance = 1;
}

namespace gui {

static void gl_after_callback(const glbinding::FunctionCall& fn) {
  for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError()) {
    std::stringstream gl_error;
    gl_error << error;
    LOG_ERROR("OpenGL: {} set error {}.", fn.function->name(), gl_error.str());
    assert(0);
  }
}

void Gui::init() {
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
    SDL_ERROR("Unable to initialize SDL");

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  m_window = SDL_CreateWindow("Pctation | OpenGL", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024,
                              512, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

  if (!m_window)
    SDL_ERROR("Unable to create window");

  m_gl_context = SDL_GL_CreateContext(m_window);

  if (!m_gl_context)
    SDL_ERROR("Unable to create GL context");

  // Try adaptive vsync first, falling back to regular vsync.
  if (SDL_GL_SetSwapInterval(-1) < 0) {
    SDL_GL_SetSwapInterval(1);
  }

  const glbinding::GetProcAddress get_proc_address = [](const char* name) {
    return reinterpret_cast<glbinding::ProcAddress>(SDL_GL_GetProcAddress(name));
  };
  glbinding::initialize(get_proc_address, false);
  //  glbinding::setCallbackMaskExcept(glbinding::CallbackMask::Before | glbinding::CallbackMask::After,
  //                                   { "glGetError" });
  glbinding::setAfterCallback(gl_after_callback);

  // Set-up imgui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  // Setup platform/renderer bindings
  ImGui_ImplSDL2_InitForOpenGL(m_window, m_gl_context);
  ImGui_ImplOpenGL3_Init("#version 330 core");
}

bool Gui::poll_events() {
  return SDL_PollEvent(&m_event);
}

bool Gui::process_events() {
  bool should_exit = false;

  ImGui_ImplSDL2_ProcessEvent(&m_event);
  if (m_event.type == SDL_QUIT)
    should_exit = true;
  if (m_event.type == SDL_WINDOWEVENT && m_event.window.event == SDL_WINDOWEVENT_CLOSE &&
      m_event.window.windowID == SDL_GetWindowID(m_window))
    should_exit = true;

  return should_exit;
}

void Gui::draw(const emulator::Emulator& emulator) {
  // Start the Dear ImGui frame
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame(m_window);
  ImGui::NewFrame();

  draw_imgui(emulator);

  // Render imgui
  ImGui::Render();
  SDL_GL_MakeCurrent(m_window, m_gl_context);
  ImGuiIO& io = ImGui::GetIO();
  glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  m_draw_counter++;
}

void Gui::swap() {
  SDL_GL_SwapWindow(m_window);
}

void Gui::draw_imgui(const emulator::Emulator& emulator) {
  if (false && ImGui::BeginMainMenuBar()) {  // TODO: Enable when it doesn't overlay with screen
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
}

void Gui::deinit() {
  SDL_GL_DeleteContext(m_gl_context);

  SDL_Quit();
}

void Gui::clear() {
  glClearColor(GUI_CLEAR_COLOR.x, GUI_CLEAR_COLOR.y, GUI_CLEAR_COLOR.z, GUI_CLEAR_COLOR.w);
  glClear(GL_COLOR_BUFFER_BIT);
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

  m_ram_memeditor.DrawWindow("RAM Contents", (MemoryEditor::u8*)ram_data.data(), RamSize, 0);
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
      ImGui::SetColumnWidth(i, column_max_chars[i] * g_char_width);
    }

    ImGui::Separator();
    for (auto i = 0; i < column_count; ++i) {
      ImGui::TextColored(GUI_TABLE_COLUMN_TITLES_COL, "%s", header_titles[i]);
      ImGui::NextColumn();
    }
    ImGui::Separator();

    // Draw rows
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
    { "E3h.0-9", "Draw Area top left X", fmt::format("{}", gpu.m_drawing_area_top_left.x) },
    { "E3h.10-18", "Draw Area top left Y", fmt::format("{}", gpu.m_drawing_area_top_left.y) },
    { "E4h.0-9", "Draw Area bott. right X", fmt::format("{}", gpu.m_drawing_area_bottom_right.x) },
    { "E4h.10-18", "Draw Area bott. right Y", fmt::format("{}", gpu.m_drawing_area_bottom_right.y) },
    { "E5h.0-10", "Drawing offset X", fmt::format("{}", gpu.m_drawing_offset.x) },
    { "E5h.10-19", "Drawing offset Y", fmt::format("{}", gpu.m_drawing_offset.y) },
  };

  ImGui::NextColumn();

  draw_register_table("GP0", gp0_rows);

  // Draw GP1 fields
  const RegisterTableEntries gp1_rows = {
    { "05h.0-10", "Display area X", fmt::format("{}", gpu.m_display_area.x) },
    { "05h.10-18", "Display area Y", fmt::format("{}", gpu.m_display_area.y) },
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
  // TODO: this broke after the swith to SDL
  return;
  // Don't want to deal with time, just update FPS overlay every X frames
  if (m_draw_counter % 20 == 0) {
    // TODO: implement FPS counter
    //    m_fps_overlay_str = fmt::format("FPS: {:0.2f}", );
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
