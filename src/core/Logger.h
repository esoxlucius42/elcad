#pragma once

// Set compile-time log level to TRACE (0) so all levels are compiled in.
// The runtime level on the sinks controls what is actually written.
// This must be defined before the first spdlog include in every translation
// unit — the CMakeLists.txt compile definition provides the same guarantee.
#ifndef SPDLOG_ACTIVE_LEVEL
#  define SPDLOG_ACTIVE_LEVEL 0
#endif

#include <spdlog/spdlog.h>
#include <string>

namespace elcad {

class Logger
{
public:
    // Initialise the rotating file logger.
    // logDir — directory where elcad.log will be written (typically next to
    // the executable).  Call once, early in Application::Application().
    static void init(const std::string& logDir);
};

} // namespace elcad

// ── Convenience macros ────────────────────────────────────────────────────────
// These wrap the SPDLOG_* macros, which automatically capture __FILE__ and
// __LINE__ so the log pattern can show the source location.
#define LOG_TRACE(...)    SPDLOG_TRACE(__VA_ARGS__)
#define LOG_DEBUG(...)    SPDLOG_DEBUG(__VA_ARGS__)
#define LOG_INFO(...)     SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...)     SPDLOG_WARN(__VA_ARGS__)
#define LOG_ERROR(...)    SPDLOG_ERROR(__VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)
