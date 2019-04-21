#pragma once

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#define LOG_TRACE SPDLOG_TRACE
#define LOG_DEBUG SPDLOG_DEBUG
#define LOG_INFO SPDLOG_INFO
#define LOG_WARN SPDLOG_WARN
#define LOG_ERROR SPDLOG_ERROR
#define LOG_CRITICAL SPDLOG_CRITICAL

#define LOG_TODO() LOG_WARN(__FUNCTION__ ": TODO")

#define LOG_CPU(...) SPDLOG_LOGGER_TRACE(logging::g_cpu_logger, __VA_ARGS__)
#define LOG_CPU_NOFMT(msg) logging::g_cpu_logger->trace(msg)

namespace logging {

extern std::shared_ptr<spdlog::logger> g_cpu_logger;

void init();

}  // namespace logging
