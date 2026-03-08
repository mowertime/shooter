#include "core/Logger.h"

namespace shooter {

Logger& getLoggerInstance() {
    static Logger s_logger;
    return s_logger;
}

Logger& Logger::get() { return getLoggerInstance(); }

const char* Logger::levelString(LogLevel l) {
    switch (l) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
    }
    return "UNKNOWN";
}

} // namespace shooter
