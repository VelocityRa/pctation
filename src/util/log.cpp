#include <util/log.hpp>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <string>
#include <vector>

constexpr auto LOG_FILENAME =
#ifdef WIN32
    L"pctation.log";
#else
    "pctation.log";
#endif

namespace logging {

void init() {
  // Set up sinks
  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_st>());
  //  sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_st>(LOG_FILENAME, true));

  // Set up logger
  auto pctation_logger = std::make_shared<spdlog::logger>("pctation", begin(sinks), end(sinks));
  pctation_logger->flush_on(spdlog::level::err);
  spdlog::flush_every(std::chrono::seconds(1));

  // Configure spdlog
  spdlog::set_default_logger(pctation_logger);
  spdlog::set_pattern("%^[--%L--] %16s:%-3# %v%$");
  spdlog::set_level(spdlog::level::debug);
}

}  // namespace logging
