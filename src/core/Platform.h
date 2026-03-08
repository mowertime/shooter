#pragma once

// ── Platform detection ────────────────────────────────────────────────────────
#if defined(_WIN32) || defined(_WIN64)
    #define SHOOTER_PLATFORM_WINDOWS 1
#elif defined(__linux__)
    #define SHOOTER_PLATFORM_LINUX 1
#elif defined(__APPLE__)
    #define SHOOTER_PLATFORM_MACOS 1
#else
    #error "Unsupported platform"
#endif

// ── Compiler helpers ──────────────────────────────────────────────────────────
#ifdef SHOOTER_PLATFORM_WINDOWS
    #define SHOOTER_FORCEINLINE __forceinline
    #define SHOOTER_NOINLINE    __declspec(noinline)
    #define SHOOTER_ALIGN(n)    __declspec(align(n))
#else
    #define SHOOTER_FORCEINLINE __attribute__((always_inline)) inline
    #define SHOOTER_NOINLINE    __attribute__((noinline))
    #define SHOOTER_ALIGN(n)    __attribute__((aligned(n)))
#endif

// ── Standard integer types ────────────────────────────────────────────────────
#include <cstdint>
#include <cstddef>

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using f32 = float;
using f64 = double;

namespace shooter {

/// Runtime platform information queried once at startup.
struct PlatformInfo {
    const char* name;           ///< Human-readable platform name
    u32  logicalCoreCount;      ///< Logical CPU core count
    u64  totalPhysicalMemoryBytes;
    u64  availablePhysicalMemoryBytes;
    bool supportsVulkan;
    bool supportsDirectX12;
};

/// Initialise platform-specific subsystems and populate \p info.
bool platformInit(PlatformInfo& info);

/// Tear down platform-specific subsystems.
void platformShutdown();

/// Returns the high-resolution timestamp in microseconds.
i64 platformGetTimeMicros();

} // namespace shooter
