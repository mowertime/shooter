#pragma once

// ---------------------------------------------------------------------------
// Platform.h — OS detection, primitive types, and utility macros
// ---------------------------------------------------------------------------

#include <cstdint>
#include <cassert>
#include <chrono>

// ---- OS detection ---------------------------------------------------------
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS 1
#elif defined(__APPLE__)
    #define PLATFORM_MAC 1
#elif defined(__linux__)
    #define PLATFORM_LINUX 1
#else
    #error "Unsupported platform"
#endif

// ---- Assertion macro -------------------------------------------------------
#ifdef NDEBUG
    #define SE_ASSERT(expr, msg) ((void)(expr))
#else
    #define SE_ASSERT(expr, msg) \
        do { \
            if (!(expr)) { \
                assert(false && msg); \
            } \
        } while (false)
#endif

// ---- Primitive type aliases ------------------------------------------------
using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i32 = int32_t;
using i64 = int64_t;
using f32 = float;
using f64 = double;

namespace shooter {

// ---------------------------------------------------------------------------
// Timer — high-resolution wall-clock timer used to measure dt and profile.
// ---------------------------------------------------------------------------
class Timer {
public:
    using Clock     = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;

    Timer() { reset(); }

    /// Reset the timer origin to now.
    void reset() { m_start = Clock::now(); }

    /// Elapsed seconds since last reset().
    [[nodiscard]] f64 elapsedSeconds() const {
        auto now = Clock::now();
        return std::chrono::duration<f64>(now - m_start).count();
    }

    /// Elapsed milliseconds since last reset().
    [[nodiscard]] f64 elapsedMilliseconds() const {
        return elapsedSeconds() * 1000.0;
    }

private:
    TimePoint m_start;
};

} // namespace shooter
