#include "core/Platform.h"

#include <chrono>
#include <thread>

#ifdef SHOOTER_PLATFORM_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#elif defined(SHOOTER_PLATFORM_LINUX)
    #include <sys/sysinfo.h>
    #include <unistd.h>
#endif

namespace shooter {

bool platformInit(PlatformInfo& info) {
    info.logicalCoreCount = static_cast<u32>(std::thread::hardware_concurrency());
    if (info.logicalCoreCount == 0) info.logicalCoreCount = 1;

#ifdef SHOOTER_PLATFORM_WINDOWS
    info.name = "Windows";

    MEMORYSTATUSEX memStatus{};
    memStatus.dwLength = sizeof(memStatus);
    GlobalMemoryStatusEx(&memStatus);
    info.totalPhysicalMemoryBytes     = static_cast<u64>(memStatus.ullTotalPhys);
    info.availablePhysicalMemoryBytes = static_cast<u64>(memStatus.ullAvailPhys);

    // Detect Vulkan support by checking for the loader DLL.
    info.supportsVulkan    = (LoadLibraryA("vulkan-1.dll") != nullptr);
    info.supportsDirectX12 = true; // Assume DX12 available on Windows 10+
#elif defined(SHOOTER_PLATFORM_LINUX)
    info.name = "Linux";

    struct sysinfo si{};
    if (sysinfo(&si) == 0) {
        info.totalPhysicalMemoryBytes     = static_cast<u64>(si.totalram) * si.mem_unit;
        info.availablePhysicalMemoryBytes = static_cast<u64>(si.freeram)  * si.mem_unit;
    }
    info.supportsVulkan    = true; // Detection deferred to renderer init
    info.supportsDirectX12 = false;
#else
    info.name = "macOS";
    info.totalPhysicalMemoryBytes     = 0;
    info.availablePhysicalMemoryBytes = 0;
    info.supportsVulkan    = false;
    info.supportsDirectX12 = false;
#endif

    return true;
}

void platformShutdown() {
    // Platform-specific cleanup (currently none required).
}

i64 platformGetTimeMicros() {
    using namespace std::chrono;
    return duration_cast<microseconds>(
        steady_clock::now().time_since_epoch()).count();
}

} // namespace shooter
