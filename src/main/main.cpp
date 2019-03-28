#include <emulator/emulator.hpp>
#include <util/log.hpp>

#include <cassert>
#include <chrono>
#include <exception>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

void init_logging() {
  // Set up sinks
  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_st>());
  sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_st>(L"pctation.log", true));

  // Set up logger
  auto pctation_logger = std::make_shared<spdlog::logger>("pcstation", begin(sinks), end(sinks));
  pctation_logger->flush_on(spdlog::level::err);
  spdlog::flush_every(std::chrono::seconds(1));

  // Configure spdlog
  spdlog::set_default_logger(pctation_logger);
  spdlog::set_pattern("%^[--%L--] %16s:%-3# %v%$");
}

int main() {
  try {
    init_logging();

    emulator::Emulator emulator("../../data/bios/SCPH101.BIN");
    emulator.run();

  } catch (const std::exception& e) {
    LOG_CRITICAL("{}", e.what());
    assert(0);
  }

  return 0;
}
