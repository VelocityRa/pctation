#include <util/log.hpp>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <initializer_list>
#include <string>
#include <vector>

namespace logging {

std::shared_ptr<spdlog::logger> g_cpu_logger;
std::shared_ptr<spdlog::logger> g_gte_logger;
std::shared_ptr<spdlog::logger> g_cdrom_logger;
std::shared_ptr<spdlog::logger> g_joypad_logger;

static constexpr auto ENABLE_FILE_LOGGING = false;

static constexpr auto LOG_FILENAME = "pctation.log";
static constexpr auto LOG_CPU_FILENAME = "pctation_cpu.log";

static constexpr auto DEFAULT_LOG_PATTERN = "%^[--%L--] %16s:%-3# %v%$";

void init() {
  // Set up sinks
  spdlog::sink_ptr cmd_sink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
  spdlog::sink_ptr file_sink_main =
      std::make_shared<spdlog::sinks::basic_file_sink_st>(LOG_FILENAME, true);
  spdlog::sink_ptr file_sink_cpu =
      std::make_shared<spdlog::sinks::basic_file_sink_st>(LOG_CPU_FILENAME, true);

  std::initializer_list<spdlog::sink_ptr> main_sinks;

  if (ENABLE_FILE_LOGGING)
    main_sinks = { cmd_sink, file_sink_main };
  else
    main_sinks = { cmd_sink };

  // Set up default logger
  auto default_logger = std::make_shared<spdlog::logger>("main", main_sinks);
  default_logger->set_level(spdlog::level::warn);
  default_logger->flush_on(spdlog::level::trace);
  default_logger->set_pattern(DEFAULT_LOG_PATTERN);
  spdlog::register_logger(default_logger);

  // Set up CPU logger
  g_cpu_logger = std::make_shared<spdlog::logger>("cpu", file_sink_cpu);
  g_cpu_logger->set_level(spdlog::level::trace);
  g_cpu_logger->flush_on(spdlog::level::trace);
  g_cpu_logger->set_pattern("%v");

  // Set up GTE logger
  g_gte_logger = std::make_shared<spdlog::logger>("gte", main_sinks);
  g_gte_logger->set_level(spdlog::level::trace);
  g_gte_logger->flush_on(spdlog::level::trace);
  g_gte_logger->set_pattern(DEFAULT_LOG_PATTERN);

  // Set up CDROM logger
  g_cdrom_logger = std::make_shared<spdlog::logger>("cdrom", main_sinks);
  g_cdrom_logger->set_level(spdlog::level::trace);
  g_cdrom_logger->flush_on(spdlog::level::trace);
  g_cdrom_logger->set_pattern(DEFAULT_LOG_PATTERN);

  // Set up Joypad logger
  g_joypad_logger = std::make_shared<spdlog::logger>("joypad", main_sinks);
  g_joypad_logger->set_level(spdlog::level::info);
  g_joypad_logger->flush_on(spdlog::level::trace);
  g_joypad_logger->set_pattern(DEFAULT_LOG_PATTERN);

  // Configure spdlog
  spdlog::set_default_logger(default_logger);
}

}  // namespace logging
