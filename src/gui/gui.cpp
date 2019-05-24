#include <gui/gui.hpp>

#include <cpu/cpu.hpp>
#include <emulator/emulator.hpp>
#include <emulator/settings.hpp>
#include <gpu/gpu.hpp>
#include <renderer/rasterizer.hpp>
#include <util/fs.hpp>
#include <util/log.hpp>

#pragma warning(disable : 4251)  // hide some glbinding warnings
#include <glbinding-aux/types_to_string.h>
#include <glbinding/Binding.h>
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
#pragma warning(default : 4251)
#include <SDL2/SDL.h>
#include <imgui.h>
#include <imgui_sdl2_gl3_backend/imgui_impl_opengl3.h>
#include <imgui_sdl2_gl3_backend/imgui_impl_sdl.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <exception>
#include <sstream>
#include <string>

#define RGBA_TO_FLOAT(r, g, b, a) ImVec4(r / 255.f, g / 255.f, b / 255.f, a / 255.f)

using namespace gl;

const auto SCREEN_SCALE = 1.5f;

const auto GUI_CLEAR_COLOR = RGBA_TO_FLOAT(46, 63, 95, 255);
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

void Gui::set_joypad(io::Joypad* joypad) {
  m_joypad = joypad;
}

void Gui::set_settings(emulator::Settings* settings) {
  m_settings = settings;
}

void Gui::set_game_title(const std::string& game_title) {
  m_game_title = game_title;

  update_window_title();
}

bool Gui::poll_events() {
  return SDL_PollEvent(&m_event);
}

GuiEvent Gui::process_events() {
  auto ret_event = GuiEvent::None;

  // Let ImGui process events
  ImGui_ImplSDL2_ProcessEvent(&m_event);

  if (m_event.type == SDL_KEYUP || m_event.type == SDL_KEYDOWN) {
    const bool was_pressed = m_event.key.state == SDL_PRESSED;
    const auto sym = m_event.key.keysym.sym;

    // Joypad events
    u8 button_index;
    switch (sym) {
      case SDLK_w: button_index = io::BTN_SELECT; break;
      case SDLK_c: button_index = io::BTN_L3; break;
      case SDLK_v: button_index = io::BTN_R3; break;
      case SDLK_2: button_index = io::BTN_START; break;
      case SDLK_UP: button_index = io::BTN_PAD_UP; break;
      case SDLK_RIGHT: button_index = io::BTN_PAD_RIGHT; break;
      case SDLK_DOWN: button_index = io::BTN_PAD_DOWN; break;
      case SDLK_LEFT: button_index = io::BTN_PAD_LEFT; break;
      case SDLK_1: button_index = io::BTN_L2; break;
      case SDLK_3: button_index = io::BTN_R2; break;
      case SDLK_q: button_index = io::BTN_L1; break;
      case SDLK_e: button_index = io::BTN_R1; break;
      case SDLK_s: button_index = io::BTN_TRIANGLE; break;
      case SDLK_x: button_index = io::BTN_CIRCLE; break;
      case SDLK_z: button_index = io::BTN_CROSS; break;
      case SDLK_a: button_index = io::BTN_SQUARE; break;
      default: button_index = io::BTN_INVALID;
    }
    if (button_index != io::BTN_INVALID) {
      m_joypad->update_button(button_index, was_pressed);
      return ret_event;
    }

    // Emulator operation events
    if (m_event.type == SDL_KEYDOWN) {
      switch (sym) {
        case SDLK_TAB:
          switch (m_settings->screen_view) {
            case emulator::View::Vram: m_settings->screen_view = emulator::View::Display; break;
            case emulator::View::Display: m_settings->screen_view = emulator::View::Vram; break;
          }
          m_settings->window_size_changed = true;
          break;
        case SDLK_g: m_settings->show_gui ^= true; break;
      }
    }
  }

  // Exit events
  process_exit_events(ret_event);

  return ret_event;
}

void Gui::imgui_start_frame() const {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame(m_window);
  ImGui::NewFrame();
}

void Gui::imgui_end_frame() const {
  ImGui::Render();
  SDL_GL_MakeCurrent(m_window, m_gl_context);
  ImGuiIO& io = ImGui::GetIO();
  glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Gui::draw(const emulator::Emulator& emulator) {
  imgui_start_frame();

  imgui_draw(emulator);

  imgui_end_frame();
}

void Gui::swap() {
  SDL_GL_SwapWindow(m_window);

  update_fps_counter();
}

void Gui::apply_settings() const {
  if (m_settings->window_size_changed) {
    m_settings->window_size_changed = false;

    f32 scale = m_settings->get_screen_scale();
    SDL_SetWindowSize(m_window, static_cast<s32>(m_settings->res_width * scale),
                      static_cast<s32>(m_settings->res_height * scale));
  }
  if (m_settings->limit_framerate_changed)
    // Limit framerate via Vsync
    SDL_GL_SetSwapInterval(m_settings->limit_framerate ? -1 : 0);
}

void Gui::deinit() {
  SDL_GL_DeleteContext(m_gl_context);
  SDL_Quit();
}

void Gui::clear() const {
  glClearColor(GUI_CLEAR_COLOR.x, GUI_CLEAR_COLOR.y, GUI_CLEAR_COLOR.z, GUI_CLEAR_COLOR.w);
  glClear(GL_COLOR_BUFFER_BIT);
}

GuiEvent Gui::process_events_file_select() const {
  auto ret_event = GuiEvent::None;

  // Let ImGui process events
  ImGui_ImplSDL2_ProcessEvent(&m_event);

  // Exit events
  process_exit_events(ret_event);

  return ret_event;
}

void Gui::process_exit_events(GuiEvent& ret_event) const {
  if (m_event.type == SDL_QUIT)
    ret_event = GuiEvent::Exit;
  else if (m_event.type == SDL_WINDOWEVENT && m_event.window.event == SDL_WINDOWEVENT_CLOSE &&
           m_event.window.windowID == SDL_GetWindowID(m_window))
    ret_event = GuiEvent::Exit;
}

void Gui::update_fps_counter() {
  ++m_fps_counter_frames;

  constexpr auto SAMPLE_INTERVAL = std::chrono::milliseconds(500);

  const auto now = std::chrono::steady_clock::now();
  const std::chrono::duration<float> interval = now - m_fps_counter_start;

  if (interval > SAMPLE_INTERVAL) {
    m_fps = m_fps_counter_frames / interval.count();
    m_fps_counter_start = now;
    m_fps_counter_frames = 0;

    update_window_title();
  }
}

void Gui::update_window_title() const {
  auto speed_percent = static_cast<u32>(m_fps / 60.f * 100.f);
  std::string window_title = "Pctation";

  window_title += fmt::format(" | {:0.2f} FPS", m_fps);

  // Don't show small deviations from full-speed
  if (99 > speed_percent || speed_percent > 100)
    window_title += fmt::format(" | {:>3d}%", speed_percent);

  if (!m_game_title.empty())
    window_title += std::string(" | ") + m_game_title;

  SDL_SetWindowTitle(m_window, window_title.c_str());
}

//
// GUI Components (imgui)
//

void Gui::imgui_draw(const emulator::Emulator& emulator) {
  if (m_settings->show_gui) {
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("Debug")) {
        ImGui::MenuItem("TTY Output", "Ctrl+T", &m_draw_tty, LOG_TTY_OUTPUT_WITH_HOOK || LOG_BIOS_CALLS);
        ImGui::MenuItem("BIOS Function Calls", "Ctrl+B", &m_draw_bios_calls, LOG_BIOS_CALLS);
        ImGui::MenuItem("RAM Contents", "Ctrl+R", &m_draw_ram);
        ImGui::MenuItem("GPU Registers", "Ctrl+U", &m_draw_gpu_registers);
        ImGui::MenuItem("CPU Registers", "Ctrl+C", &m_draw_cpu_registers);
        ImGui::MenuItem("GP0 Commands", "Ctrl+C", &m_draw_cpu_registers, gpu::GP0_DEBUG_RECORD);
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Settings")) {
        ImGui::PushItemWidth(140);
        // Screen view
        auto screen_view_old = m_settings->screen_view;
        const char* const items_screen_view[] = { "VRAM", "Display" };
        ImGui::Text("View ");
        ImGui::SameLine();
        ImGui::Combo("##screen_view", (s32*)&m_settings->screen_view, items_screen_view,
                     ARRAYSIZE(items_screen_view));
        m_settings->window_size_changed = (screen_view_old != m_settings->screen_view);

        // Screen scale
        auto screen_scale_old = m_settings->screen_scale;
        const char* const items_screen_scale[] = { "1x", "1.5x", "2x", "3x", "4x" };
        ImGui::Text("Scale");
        ImGui::SameLine();
        ImGui::Combo("##screen_scale", (s32*)&m_settings->screen_scale, items_screen_scale,
                     ARRAYSIZE(items_screen_scale));
        if (!m_settings->window_size_changed)
          m_settings->window_size_changed = (screen_scale_old != m_settings->screen_scale);

        auto limit_framerate_old = m_settings->limit_framerate;
        // Frame limiter
        ImGui::MenuItem("Throttle FPS", "Ctrl+F", &m_settings->limit_framerate);
        m_settings->limit_framerate_changed = (limit_framerate_old != m_settings->limit_framerate);

        // Gui visibility
        ImGui::MenuItem("Show GUI", "Ctrl+G", &m_settings->show_gui);

        ImGui::MenuItem("Trace CPU", "Ctrl+P", &m_settings->log_trace_cpu);

        ImGui::PopItemWidth();
        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
    }

    if (m_draw_tty && (LOG_TTY_OUTPUT_WITH_HOOK || LOG_BIOS_CALLS))
      draw_window_log("TTY Output", m_draw_tty, m_tty_autoscroll, emulator.cpu().m_tty_out_log.c_str());
    if (m_draw_bios_calls && LOG_BIOS_CALLS)
      draw_window_log("BIOS Function Calls", m_draw_bios_calls, m_bios_calls_autoscroll,
                      emulator.cpu().m_bios_calls_log.c_str());
    if (m_draw_ram)
      draw_window_ram(emulator.ram().data());
    if (m_draw_gpu_registers)
      draw_window_gpu_registers(emulator.gpu());
    if (m_draw_cpu_registers)
      draw_window_cpu_registers(emulator.cpu());
    if (m_draw_gp0_commands)
      draw_window_gp0_commands(emulator.gpu());
  }
}

void Gui::draw_window_log(const char* title,
                          bool& should_draw,
                          bool& should_autoscroll,
                          const char* text_contents) const {
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

void Gui::draw_file_select(gui::Gui& gui, std::string& exe_path, std::string& cdrom_path) {
  auto event = gui::GuiEvent::None;

  while (true) {
    bool selected{};

    while (gui.poll_events()) {
      event = gui.process_events_file_select();

      if (event == gui::GuiEvent::GameSelected || event == gui::GuiEvent::Exit) {
        selected = true;
        return;
      }
    }
    gui.clear();

    imgui_start_frame();

    if (gui.draw_window_exe_select(exe_path))
      selected = true;
    if (gui.draw_window_cdrom_select(cdrom_path))
      selected = true;

    imgui_end_frame();

    gui.swap();

    if (selected)
      return;
  }
}

bool Gui::draw_window_exe_select(std::string& exe_path) const {
  const auto EXE_PATH = "data/exe";

  bool selected = false;

  if (ImGui::Begin("PSX-EXE Explorer")) {
    for (auto& p : fs::recursive_directory_iterator(EXE_PATH)) {
      const auto ext = p.path().extension();
      if (ext == ".exe" || ext == ".psx") {
        const auto path = p.path().string();
        const auto rel_path = path.substr(sizeof(EXE_PATH));

        if (ImGui::Selectable(rel_path.c_str())) {
          exe_path = path;
          selected = true;
        }
      }
    }
  }
  ImGui::End();

  return selected;
}

bool Gui::draw_window_cdrom_select(std::string& cdrom_path) const {
  const auto CDROM_PATH = "D:\\Nikos\\PSX\\Games";

  bool selected = false;

  auto search_for_ext = [&](const fs::directory_entry& dir, std::vector<const char*> extensions,
                            bool select_path) -> bool {
    for (auto& f : fs::recursive_directory_iterator(dir)) {
      const auto file_path = f.path().string();
      auto ext = f.path().extension().string();
      std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

      if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end()) {
        if (select_path)
          cdrom_path = file_path;
        return true;
      }
    }
    return false;
  };

  if (ImGui::Begin("CD-ROM Explorer")) {
    if (ImGui::Selectable("None"))
      selected = true;

    for (auto& p : fs::recursive_directory_iterator(CDROM_PATH)) {
      if (fs::is_directory(p)) {
        const auto dir_path = p.path().string();
        const auto rel_dir_path = dir_path.substr(sizeof(CDROM_PATH));

        if (search_for_ext(p, { ".bin", ".iso" }, false)) {  // todo: ".cue"
          if (ImGui::Selectable(rel_dir_path.c_str())) {
            // First look for a cue sheet
            // selected = search_for_ext(p, { ".cue"}, true);

            // If we found nothing, fall back to binary formats
            if (!selected)
              selected = search_for_ext(p, { ".bin", ".iso" }, true);

            if (selected)
              LOG_INFO("Opening file: {}", cdrom_path);
          }
        }
      }
    }
  }

  ImGui::End();

  return selected;
}

void Gui::draw_window_gp0_commands(const gpu::Gpu& gpu) {
  using namespace renderer::rasterizer;

  auto draw_positions = [](Position4& positions, bool is_quad) {
    if (ImGui::TreeNodeEx("Positions", ImGuiTreeNodeFlags_DefaultOpen)) {
      s32 pos_x[4] = { (s32)positions[0].x, (s32)positions[1].x, (s32)positions[2].x,
                       (s32)positions[3].x };
      if (is_quad)
        ImGui::SliderInt4("X", pos_x, 0, gpu::VRAM_WIDTH, "%d");
      else
        ImGui::SliderInt3("X", pos_x, 0, gpu::VRAM_WIDTH, "%d");
      s32 pos_y[4] = { (s32)positions[0].y, (s32)positions[1].y, (s32)positions[2].y,
                       (s32)positions[3].y };
      if (is_quad)
        ImGui::SliderInt4("Y", pos_y, 0, gpu::VRAM_HEIGHT, "%d");
      else
        ImGui::SliderInt3("Y", pos_y, 0, gpu::VRAM_HEIGHT, "%d");
      ImGui::TreePop();
    }
  };

  auto draw_colors = [](Color colors[], bool is_flat, u32 vertex_count) {
    if (ImGui::TreeNodeEx("Colors", ImGuiTreeNodeFlags_DefaultOpen)) {
      const auto color_count = is_flat ? 1 : vertex_count;
      for (u32 i = 0; i < color_count; ++i) {
        float col[3] = { (float)colors[i].r / 255.f, (float)colors[i].g / 255.f,
                         (float)colors[i].b / 255.f };
        ImGui::ColorEdit3("", col,
                          ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs |
                              ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoOptions |
                              ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_RGB);
        ImGui::SameLine();
      }
      ImGui::NewLine();
      ImGui::TreePop();
    }
  };

  auto draw_texcoords = [](Texcoord4& texcoords, bool is_quad) {
    if (ImGui::TreeNodeEx("Texture Coordinates", ImGuiTreeNodeFlags_DefaultOpen)) {
      s32 texcoord_x[4] = { (s32)texcoords[0].x, (s32)texcoords[1].x, (s32)texcoords[2].x,
                            (s32)texcoords[3].x };
      if (is_quad)
        ImGui::SliderInt4("X", texcoord_x, 0, gpu::VRAM_WIDTH, "%d");
      else
        ImGui::SliderInt3("X", texcoord_x, 0, gpu::VRAM_WIDTH, "%d");
      s32 texcoord_y[4] = { (s32)texcoords[0].y, (s32)texcoords[1].y, (s32)texcoords[2].y,
                            (s32)texcoords[3].y };
      if (is_quad)
        ImGui::SliderInt4("Y", texcoord_y, 0, gpu::VRAM_HEIGHT, "%d");
      else
        ImGui::SliderInt3("Y", texcoord_y, 0, gpu::VRAM_HEIGHT, "%d");
      ImGui::TreePop();
    }
  };

  auto draw_size = [](Size& size) {
    if (ImGui::TreeNodeEx("Size", ImGuiTreeNodeFlags_DefaultOpen)) {
      s32 sz[2] = { (s32)size.width, size.height };
      ImGui::SliderInt("Width", &sz[0], 0, gpu::VRAM_WIDTH);
      ImGui::SliderInt("Height", &sz[1], 0, gpu::VRAM_HEIGHT);
      ImGui::TreePop();
    }
  };

  auto draw_misc_flags = [](bool is_textured, bool is_raw, bool is_flat) {
    if (ImGui::TreeNodeEx("Other", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Columns(3);
      ImGui::Checkbox("Textured", &is_textured);
      ImGui::NextColumn();
      ImGui::Checkbox("Raw", &is_raw);
      ImGui::NextColumn();
      ImGui::Checkbox("Flat", &is_flat);
      ImGui::Columns(1);
      ImGui::TreePop();
    }
  };

  auto draw_poly_overlay = [this](Position4& positions, bool is_quad) {
    auto draw_list = ImGui::GetWindowDrawList();
    draw_list->PushClipRectFullScreen();  // TODO: clip to display area
    const auto overlay_color = 0x00FFFFFF | m_draw_gp0_overlay_alpha << 24;

    const auto screen_scale = m_settings->get_screen_scale();

    if (is_quad)
      draw_list->AddQuadFilled({ positions[0].x * screen_scale, positions[0].y * screen_scale },
                               { positions[1].x * screen_scale, positions[1].y * screen_scale },
                               { positions[3].x * screen_scale, positions[3].y * screen_scale },
                               { positions[2].x * screen_scale, positions[2].y * screen_scale },
                               overlay_color);
    else
      draw_list->AddTriangleFilled({ positions[0].x * screen_scale, positions[0].y * screen_scale },
                                   { positions[1].x * screen_scale, positions[1].y * screen_scale },
                                   { positions[2].x * screen_scale, positions[2].y * screen_scale },
                                   overlay_color);
    draw_list->PopClipRect();
  };

  if (ImGui::Begin("GP0 Commands", &m_draw_gp0_commands)) {
    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 4);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 0);

    // Pulse overlay alpha
    m_draw_gp0_overlay_alpha += m_draw_gp0_overlay_rising ? 10 : -10;
    if (m_draw_gp0_overlay_alpha == 250)
      m_draw_gp0_overlay_rising = false;
    else if (m_draw_gp0_overlay_alpha == 0)
      m_draw_gp0_overlay_rising = true;

    // For each frame (-1 for the latest one)
    for (s32 cmds_i = -1; cmds_i < (s32)gpu.m_gp0_cmds_record.size(); ++cmds_i) {
      gpu::Gpu::Gp0CmdDebugRecordsFrame const* frame_cmds{};
      std::string frame_str;

      // Before every frame show the latest one
      if (cmds_i == -1) {
        // Find last frame that has commands
        for (auto cmds_it = gpu.m_gp0_cmds_record.crbegin(); cmds_it != gpu.m_gp0_cmds_record.crend();
             ++cmds_it) {
          if (cmds_it->size() != 0) {
            frame_cmds = &(*cmds_it);
            break;
          }
        }
        if (!frame_cmds)
          continue;

        frame_str = fmt::format("Frame [latest]");
        ImGui::Spacing();
      } else {
        frame_cmds = &gpu.m_gp0_cmds_record[cmds_i];
        frame_str = fmt::format("Frame #{}", cmds_i);
      }

      const auto cmds_count = frame_cmds->size();

      // Skip frames with no commands
      if (cmds_count == 0)
        continue;

      if (ImGui::TreeNode(frame_str.c_str())) {
        // For each command
        for (u32 cmd_i = 0; cmd_i < cmds_count; ++cmd_i) {
          const auto& cmd = (*frame_cmds)[cmd_i];
          const auto cmd_id = std::to_string(cmd_i);
          const auto cmd_type = cmd.type;
          const auto& cmd_words = cmd.cmd;
          const auto cmd_word_first = cmd_words.front();
          const u8 opcode = cmd_word_first >> 24;

          std::string cmd_string_title;
          if (cmd_type == gpu::Gp0CommandType::DrawPolygon) {
            const auto is_quad = DrawCommand{ opcode }.polygon.is_quad();
            cmd_string_title = is_quad ? "Draw Quad" : "Draw Triangle";
          } else
            cmd_string_title = gpu::gp0_cmd_type_to_str(cmd_type);

          const auto cmd_string = fmt::format("Command #{}: {}", cmd_i, cmd_string_title);

          if (ImGui::TreeNode(cmd_id.c_str(), cmd_string.c_str())) {
            switch (cmd_type) {
              case gpu::Gp0CommandType::DrawLine: break;
              case gpu::Gp0CommandType::DrawRectangle: {
                auto rectangle = DrawCommand{ opcode }.rectangle;

                Position4 positions{};
                Color4 colors{};
                TextureInfo tex_info{};
                Size size{};

                gpu.m_rasterizer.extract_draw_data_rectangle(rectangle, cmd_words, positions, colors,
                                                             tex_info, size);

                // Positions
                const auto is_quad = true;
                draw_positions(positions, is_quad);

                // Sizes
                draw_size(size);

                // Colors
                bool is_flat = true;
                const u32 vertex_count = 4;
                draw_colors(colors.data(), is_flat, vertex_count);

                bool is_textured = (bool)rectangle.texture_mapping;
                if (is_textured)
                  draw_texcoords(tex_info.uv, is_quad);

                // Misc
                bool is_raw = (rectangle.texture_mode == DrawCommand::TextureMode::Raw);
                draw_misc_flags(is_textured, is_raw, is_flat);

                // Draw overlay of primitive
                draw_poly_overlay(positions, is_quad);

                break;
              }
              case gpu::Gp0CommandType::DrawPolygon: {
                auto polygon = DrawCommand{ opcode }.polygon;

                Position4 positions;
                Color4 colors;
                TextureInfo tex_info{};

                gpu.m_rasterizer.extract_draw_data_polygon(polygon, cmd_words, positions, colors,
                                                           tex_info);

                const auto vertex_count = polygon.get_vertex_count();

                // Positions
                const auto is_quad = polygon.is_quad();
                draw_positions(positions, is_quad);

                // Colors
                bool is_flat = (polygon.shading == DrawCommand::Shading::Flat);
                draw_colors(colors.data(), is_flat, vertex_count);

                bool is_textured = (bool)polygon.texture_mapping;
                if (is_textured)
                  draw_texcoords(tex_info.uv, is_quad);

                // Misc
                bool is_raw = (polygon.texture_mode == DrawCommand::TextureMode::Raw);
                draw_misc_flags(is_textured, is_raw, is_flat);

                // Draw overlay of primitive
                draw_poly_overlay(positions, is_quad);

                break;
              }
              case gpu::Gp0CommandType::FillRectangleInVram: break;
              case gpu::Gp0CommandType::CopyCpuToVram: break;
              case gpu::Gp0CommandType::CopyCpuToVramTransferring: break;
              case gpu::Gp0CommandType::CopyVramToCpu: break;
            }

            ImGui::TreePop();
            ImGui::Separator();
          }
        }
        ImGui::TreePop();
      }
    }
    ImGui::PopStyleVar(2);
  }
  ImGui::End();
}

template <size_t RamSize>
void Gui::draw_window_ram(const std::array<byte, RamSize>& ram_data) {
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

void Gui::draw_window_gpu_registers(const gpu::Gpu& gpu) {
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
  const char* tex_page_colors_str[] = { "4bit", "8bit", "15bit", "15bitAlt" };
  const char* video_mode_str[] = { "NTSC", "PAL" };
  const char* disp_color_depth_str[] = { "15bit", "24bit" };
  const char* dma_direction_str[] = { "Off", "???", "CPUtoGP0", "VRMtoCPU" };
  const char* interlace_type_str[] = { "Even", "Odd" };

  auto resolution_str = [&gpu]() {
    const auto res = gpu.get_resolution();
    return fmt::format("{}x{}", res.width, res.height);
  };

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
    { "16-19", "Resolution", fmt::format("{}", resolution_str()) },
    { "20", "Video mode", video_mode_str[gs.video_mode] },
    { "21", "Display Color Depth", disp_color_depth_str[gs.disp_color_depth] },
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

void Gui::draw_window_cpu_registers(const cpu::Cpu& cpu) {
  if (ImGui::Begin("CPU Registers", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("pc: %08X", cpu.m_pc_current);
    ImGui::Text("ra: %08X", cpu.gpr(31));

    if (ImGui::CollapsingHeader("Show All", { true })) {
      auto print_reg = [&cpu](u32 reg_idx) {
        ImGui::Text("%s: %08X", cpu::register_to_str(reg_idx), cpu.gpr(reg_idx));
      };

      u32 i = 1;
      print_reg(i++);
      while (true) {
        print_reg(i++);
        ImGui::SameLine();
        if (i == 31)
          break;
        print_reg(i++);
      }
    }
  }

  ImGui::End();
}

}  // namespace gui
