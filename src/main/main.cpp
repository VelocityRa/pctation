#include <emulator/emulator.hpp>
#include <gui/gui.hpp>
#include <util/log.hpp>

#include <cassert>
#include <exception>
#include <memory>

#include <bigg.hpp>

class PctationApp : public bigg::Application {
  void initialize(s32 _argc, char** _argv) override {
    const auto renderer = bgfx::getRendererName(bgfx::getRendererType());
    const auto window_title = fmt::format("pctation | {}", renderer);

    glfwSetWindowTitle(mWindow, window_title.c_str());
    bgfx::setDebug(BGFX_DEBUG_TEXT);
  }

  void onReset() override {
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x85144bFF, 1.0f, 0);
    bgfx::setViewRect(0, 0, 0, uint16_t(getWidth()), uint16_t(getHeight()));

    m_emulator = std::make_unique<emulator::Emulator>("../../data/bios/SCPH1001.BIN");
  }

  void update(float dt) override {
    bgfx::touch(0);

    ImGui::ShowDemoWindow();
    m_gui.show(*m_emulator);

    m_emulator->advance_frame();
  }

 private:
  std::unique_ptr<emulator::Emulator> m_emulator;
  gui::Gui m_gui;
};

// Entry point
s32 main(s32 argc, char** argv) {
  try {
    logging::init();

    PctationApp app;
    return app.run(argc, argv);
  } catch (const std::exception& e) {
    LOG_CRITICAL("{}", e.what());
    assert(0);
  }

  return 0;
}
