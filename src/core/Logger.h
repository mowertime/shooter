#pragma once

#include "core/Platform.h"
#include <string>
#include <cstdio>

namespace shooter {

enum class LogLevel : u8 {
    Debug   = 0,
    Info    = 1,
    Warning = 2,
    Error   = 3,
    Fatal   = 4,
};

class Logger {
public:
    static Logger& get();

    void setMinLevel(LogLevel level) { m_minLevel = level; }

    template<typename... Args>
    void log(LogLevel level, const char* category,
             const char* fmt, Args&&... args);

    // Zero-argument overload avoids -Wformat-security warning.
    void log(LogLevel level, const char* category, const char* msg);

private:
    Logger() = default;  // Use Logger::get() to access the singleton
    LogLevel m_minLevel{LogLevel::Debug};
    static const char* levelString(LogLevel l);
    friend Logger& getLoggerInstance(); // Allow .cpp to access constructor
};

// ── Lightweight format helper (printf-based) ──────────────────────────────────

inline void Logger::log(LogLevel level, const char* category, const char* msg) {
    if (static_cast<u8>(level) < static_cast<u8>(m_minLevel)) return;
    const char* lvl = levelString(level);
    std::fprintf(stdout, "[%-7s][%-12s] %s\n", lvl, category, msg);
    std::fflush(stdout);
}

template<typename... Args>
void Logger::log(LogLevel level, const char* category,
                 const char* fmt, Args&&... args) {
    if (static_cast<u8>(level) < static_cast<u8>(m_minLevel)) return;

    char buf[2048];
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    std::snprintf(buf, sizeof(buf), fmt, std::forward<Args>(args)...);

    const char* lvl = levelString(level);
    std::fprintf(stdout, "[%-7s][%-12s] %s\n", lvl, category, buf);
    std::fflush(stdout);
}

// ── Convenience macros ────────────────────────────────────────────────────────
#define LOG_DEBUG(cat, fmt, ...) \
    shooter::Logger::get().log(shooter::LogLevel::Debug,   cat, fmt, ##__VA_ARGS__)
#define LOG_INFO(cat, fmt, ...) \
    shooter::Logger::get().log(shooter::LogLevel::Info,    cat, fmt, ##__VA_ARGS__)
#define LOG_WARN(cat, fmt, ...) \
    shooter::Logger::get().log(shooter::LogLevel::Warning, cat, fmt, ##__VA_ARGS__)
#define LOG_ERROR(cat, fmt, ...) \
    shooter::Logger::get().log(shooter::LogLevel::Error,   cat, fmt, ##__VA_ARGS__)
#define LOG_FATAL(cat, fmt, ...) \
    shooter::Logger::get().log(shooter::LogLevel::Fatal,   cat, fmt, ##__VA_ARGS__)

} // namespace shooter
