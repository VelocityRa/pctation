#pragma once

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#define LOG_TRACE SPDLOG_TRACE
#define LOG_DEBUG SPDLOG_DEBUG
#define LOG_INFO SPDLOG_INFO
#define LOG_WARN SPDLOG_WARN
#define LOG_ERROR SPDLOG_ERROR
#define LOG_CRITICAL SPDLOG_CRITICAL

#define LOG_TODO() LOG_WARN(__FUNCTION__ ": TODO")

#define LOG_TRACE_CPU(...) SPDLOG_LOGGER_TRACE(logging::g_cpu_logger, __VA_ARGS__)
#define LOG_TRACE_CPU_NOFMT(msg) logging::g_cpu_logger->trace(msg)

#define LOG_TRACE_GTE(...) SPDLOG_LOGGER_TRACE(logging::g_gte_logger, __VA_ARGS__)
#define LOG_DEBUG_GTE(...) SPDLOG_LOGGER_DEBUG(logging::g_gte_logger, __VA_ARGS__)

#define LOG_TRACE_CDROM(...) SPDLOG_LOGGER_TRACE(logging::g_cdrom_logger, __VA_ARGS__)
#define LOG_DEBUG_CDROM(...) SPDLOG_LOGGER_DEBUG(logging::g_cdrom_logger, __VA_ARGS__)
#define LOG_INFO_CDROM(...) SPDLOG_LOGGER_INFO(logging::g_cdrom_logger, __VA_ARGS__)
#define LOG_WARN_CDROM(...) SPDLOG_LOGGER_WARN(logging::g_cdrom_logger, __VA_ARGS__)
#define LOG_ERROR_CDROM(...) SPDLOG_LOGGER_ERROR(logging::g_cdrom_logger, __VA_ARGS__)
#define LOG_CRITICAL_CDROM(...) SPDLOG_LOGGER_CRITICAL(logging::g_cdrom_logger, __VA_ARGS__)

namespace logging {

extern std::shared_ptr<spdlog::logger> g_cpu_logger;
extern std::shared_ptr<spdlog::logger> g_gte_logger;
extern std::shared_ptr<spdlog::logger> g_cdrom_logger;

void init();

}  // namespace logging
