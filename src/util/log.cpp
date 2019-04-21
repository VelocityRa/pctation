#include <util/log.hpp>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <string>
#include <vector>

// TODO: Clean up
#ifdef WIN32
constexpr auto LOG_FILENAME = L"pctation.log";
constexpr auto LOG_CPU_FILENAME = L"pctation_cpu.log";
#else
constexpr auto LOG_FILENAME = "pctation.log";
constexpr auto LOG_CPU_FILENAME = "pctation_cpu.log";
#endif

namespace logging {

std::shared_ptr<spdlog::logger> g_cpu_logger;

void init() {
  // Set up sinks
  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_st>());
  sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_st>(LOG_FILENAME, true));

  // Set up generic logger
  auto pctation_logger = std::make_shared<spdlog::logger>("pctation", begin(sinks), end(sinks));
  pctation_logger->set_level(spdlog::level::debug);
  pctation_logger->flush_on(spdlog::level::debug);
  pctation_logger->set_pattern("%^[--%L--] %16s:%-3# %v%$");
  spdlog::register_logger(pctation_logger);

  // Set up CPU logger
  g_cpu_logger = spdlog::basic_logger_st("pctation_cpu", LOG_CPU_FILENAME, true);
  g_cpu_logger->set_level(spdlog::level::trace);
  g_cpu_logger->flush_on(spdlog::level::trace);
  g_cpu_logger->set_pattern("%v");

  // Configure spdlog
  spdlog::set_default_logger(pctation_logger);
}

}  // namespace logging
