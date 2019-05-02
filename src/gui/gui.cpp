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
#include <chrono>
#include <exception>
#include <sstream>

#define RGBA_TO_FLOAT(r, g, b, a) ImVec4(r / 255.f, g / 255.f, b / 255.f, a / 255.f)

using namespace gl;

const auto SCREEN_SCALE = 1.5;

const auto GUI_CLEAR_COLOR = RGBA_TO_FLOAT(133, 20, 75, 255);
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

void init_theme() {
  // cherry colors, 3 intensities
#define HI(v) ImVec4(0.502f, 0.075f, 0.256f, v)
#define MED(v) ImVec4(0.455f, 0.198f, 0.301f, v)
#define LOW(v) ImVec4(0.232f, 0.201f, 0.271f, v)
// backgrounds (@todo: complete with BG_MED, BG_LOW)
#define BG(v) ImVec4(0.200f, 0.220f, 0.270f, v)
// text
#define TXT(v) ImVec4(0.860f, 0.930f, 0.890f, v)

  auto& style = ImGui::GetStyle();
  style.Colors[ImGuiCol_Text] = TXT(0.78f);
  style.Colors[ImGuiCol_TextDisabled] = TXT(0.28f);
  style.Colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
  style.Colors[ImGuiCol_ChildWindowBg] = BG(0.58f);
  style.Colors[ImGuiCol_PopupBg] = BG(0.9f);
  style.Colors[ImGuiCol_Border] = ImVec4(0.31f, 0.31f, 1.00f, 0.00f);
  style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  style.Colors[ImGuiCol_FrameBg] = BG(1.00f);
  style.Colors[ImGuiCol_FrameBgHovered] = MED(0.78f);
  style.Colors[ImGuiCol_FrameBgActive] = MED(1.00f);
  style.Colors[ImGuiCol_TitleBg] = LOW(1.00f);
  style.Colors[ImGuiCol_TitleBgActive] = HI(1.00f);
  style.Colors[ImGuiCol_TitleBgCollapsed] = BG(0.75f);
  style.Colors[ImGuiCol_MenuBarBg] = BG(0.47f);
  style.Colors[ImGuiCol_ScrollbarBg] = BG(1.00f);
  style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.09f, 0.15f, 0.16f, 1.00f);
  style.Colors[ImGuiCol_ScrollbarGrabHovered] = MED(0.78f);
  style.Colors[ImGuiCol_ScrollbarGrabActive] = MED(1.00f);
  style.Colors[ImGuiCol_CheckMark] = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
  style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
  style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
  style.Colors[ImGuiCol_Button] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
  style.Colors[ImGuiCol_ButtonHovered] = MED(0.86f);
  style.Colors[ImGuiCol_ButtonActive] = MED(1.00f);
  style.Colors[ImGuiCol_Header] = MED(0.76f);
  style.Colors[ImGuiCol_HeaderHovered] = MED(0.86f);
  style.Colors[ImGuiCol_HeaderActive] = HI(1.00f);
  style.Colors[ImGuiCol_Column] = ImVec4(0.14f, 0.16f, 0.19f, 1.00f);
  style.Colors[ImGuiCol_ColumnHovered] = MED(0.78f);
  style.Colors[ImGuiCol_ColumnActive] = MED(1.00f);
  style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
  style.Colors[ImGuiCol_ResizeGripHovered] = MED(0.78f);
  style.Colors[ImGuiCol_ResizeGripActive] = MED(1.00f);
  style.Colors[ImGuiCol_PlotLines] = TXT(0.63f);
  style.Colors[ImGuiCol_PlotLinesHovered] = MED(1.00f);
  style.Colors[ImGuiCol_PlotHistogram] = TXT(0.63f);
  style.Colors[ImGuiCol_PlotHistogramHovered] = MED(1.00f);
  style.Colors[ImGuiCol_TextSelectedBg] = MED(0.43f);
  style.Colors[ImGuiCol_ModalWindowDarkening] = BG(0.73f);

  style.WindowPadding = ImVec2(6, 4);
  style.WindowRounding = 0.0f;
  style.FramePadding = ImVec2(5, 2);
  style.FrameRounding = 3.0f;
  style.ItemSpacing = ImVec2(7, 1);
  style.ItemInnerSpacing = ImVec2(1, 1);
  style.TouchExtraPadding = ImVec2(0, 0);
  style.IndentSpacing = 6.0f;
  style.ScrollbarSize = 12.0f;
  style.ScrollbarRounding = 16.0f;
  style.GrabMinSize = 20.0f;
  style.GrabRounding = 2.0f;

  style.WindowTitleAlign.x = 0.50f;

  style.Colors[ImGuiCol_Border] = ImVec4(0.539f, 0.479f, 0.255f, 0.162f);
  style.FrameBorderSize = 0.0f;
  style.WindowBorderSize = 1.0f;
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

  m_window = SDL_CreateWindow("Pctation | OpenGL", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              static_cast<s32>(gpu::VRAM_WIDTH * SCREEN_SCALE),
                              static_cast<s32>(gpu::VRAM_HEIGHT * SCREEN_SCALE),
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

  if (!m_window)
    SDL_ERROR("Unable to create window");

  m_gl_context = SDL_GL_CreateContext(m_window);

  if (!m_gl_context)
    SDL_ERROR("Unable to create GL context");

  // Don't use Vsync
  SDL_GL_SetSwapInterval(0);

  const glbinding::GetProcAddress get_proc_address = [](const char* name) {
    return reinterpret_cast<glbinding::ProcAddress>(SDL_GL_GetProcAddress(name));
  };
  glbinding::initialize(get_proc_address, false);

  // On Debug builds, check every OpenGL call
#if !NDEBUG
  auto gl_after_callback = [](const glbinding::FunctionCall& fn) {
    for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError()) {
      std::stringstream gl_error;
      gl_error << error;
      LOG_ERROR("OpenGL: {} error {}.", fn.function->name(), gl_error.str());
      assert(0);
    }
  };

  glbinding::setCallbackMaskExcept(glbinding::CallbackMask::After, { "glGetError" });
  glbinding::setAfterCallback(gl_after_callback);
#endif

  // Set-up imgui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  init_theme();

  // Setup platform/renderer bindings
  ImGui_ImplSDL2_InitForOpenGL(m_window, m_gl_context);
  ImGui_ImplOpenGL3_Init("#version 330 core");

  // Start FPS counter for first frame
  m_fps_counter_start = std::chrono::steady_clock::now();
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
}

void Gui::swap() {
  SDL_GL_SwapWindow(m_window);

  update_fps_counter();
}

void Gui::update_fps_counter() {
  ++m_fps_counter_frames;

  constexpr auto SAMPLE_INTERVAL = std::chrono::milliseconds(500);

  const auto now = std::chrono::steady_clock::now();
  const std::chrono::duration<double> interval = now - m_fps_counter_start;

  if (interval > SAMPLE_INTERVAL) {
    m_fps = m_fps_counter_frames / interval.count();
    m_fps_counter_start = now;
    m_fps_counter_frames = 0;

    update_window_title();
  }
}

void Gui::update_window_title() {
  std::string window_title = fmt::format("Pctation | OpenGL | {:0.2f} FPS", m_fps);

  SDL_SetWindowTitle(m_window, window_title.c_str());
}

void Gui::draw_imgui(const emulator::Emulator& emulator) {
  if (false && ImGui::BeginMainMenuBar()) {  // TODO: Enable when it doesn't overlay with screen
    if (ImGui::BeginMenu("Debug")) {
      ImGui::MenuItem("TTY Output", "Ctrl+T", &m_draw_tty, LOG_TTY_OUTPUT_WITH_HOOK);
      ImGui::MenuItem("BIOS Function Calls", "Ctrl+B", &m_draw_bios_calls, LOG_BIOS_CALLS);
      ImGui::MenuItem("RAM Contents", "Ctrl+R", &m_draw_ram);
      ImGui::MenuItem("GPU Registers", "Ctrl+G", &m_draw_gpu_registers);
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }

  if (m_draw_tty && (LOG_TTY_OUTPUT_WITH_HOOK || LOG_BIOS_CALLS))
    draw_dialog_log("TTY Output", m_draw_tty, m_tty_autoscroll, emulator.cpu().m_tty_out_log.c_str());
  if (m_draw_bios_calls && LOG_BIOS_CALLS)
    draw_dialog_log("BIOS Function Calls", m_draw_bios_calls, m_bios_calls_autoscroll,
                    emulator.cpu().m_bios_calls_log.c_str());
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

void Gui::draw_dialog_log(const char* title,
                          bool& should_draw,
                          bool& should_autoscroll,
                          const char* text_contents) {
  // Window style
  ImGui::SetNextWindowSize(ImVec2(470, 300), ImGuiCond_FirstUseEver);

  // Window creation
  if (!ImGui::Begin(title, &should_draw)) {
    ImGui::End();
    return;
  }

  // Options above log text
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
  ImGui::Checkbox("Auto-scroll", &should_autoscroll);
  ImGui::PopStyleVar();
  ImGui::Spacing();
  ImGui::Separator();

  // Text contents
  ImGui::BeginChild(title, ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
  ImGui::TextUnformatted(text_contents);

  if (should_autoscroll)
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

}  // namespace gui
