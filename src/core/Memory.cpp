#include "core/Memory.h"

#include <atomic>
#include <cstdlib>
#include <cassert>
#include <array>

namespace shooter {

// ── GeneralAllocator ──────────────────────────────────────────────────────────

struct GeneralAllocatorState {
    std::atomic<u64> bytesAllocated{0};
    std::atomic<u64> allocationCount{0};
};

static GeneralAllocatorState s_allocState;
static GeneralAllocator      s_generalAlloc;

GeneralAllocator& GeneralAllocator::get() { return s_generalAlloc; }

void* GeneralAllocator::alloc(u64 bytes, u32 alignment) {
#ifdef SHOOTER_PLATFORM_WINDOWS
    void* ptr = _aligned_malloc(static_cast<size_t>(bytes),
                                static_cast<size_t>(alignment));
#else
    void* ptr = nullptr;
    if (posix_memalign(&ptr, static_cast<size_t>(alignment),
                       static_cast<size_t>(bytes)) != 0) {
        ptr = nullptr;
    }
#endif
    if (ptr) {
        s_allocState.bytesAllocated.fetch_add(bytes, std::memory_order_relaxed);
        s_allocState.allocationCount.fetch_add(1, std::memory_order_relaxed);
    }
    return ptr;
}

void GeneralAllocator::free(void* ptr) {
    if (!ptr) return;
    s_allocState.allocationCount.fetch_sub(1, std::memory_order_relaxed);
#ifdef SHOOTER_PLATFORM_WINDOWS
    _aligned_free(ptr);
#else
    ::free(ptr);
#endif
}

void* GeneralAllocator::realloc(void* ptr, u64 newBytes, u32 alignment) {
#ifdef SHOOTER_PLATFORM_WINDOWS
    return _aligned_realloc(ptr, static_cast<size_t>(newBytes),
                            static_cast<size_t>(alignment));
#else
    // POSIX has no aligned_realloc; fall back to alloc+copy+free.
    void* newPtr = alloc(newBytes, alignment);
    if (ptr && newPtr) {
        std::memcpy(newPtr, ptr, static_cast<size_t>(newBytes));
        free(ptr);
    }
    return newPtr;
#endif
}

u64 GeneralAllocator::bytesAllocated()  const {
    return s_allocState.bytesAllocated.load(std::memory_order_relaxed);
}
u64 GeneralAllocator::allocationCount() const {
    return s_allocState.allocationCount.load(std::memory_order_relaxed);
}

// ── LinearAllocator ───────────────────────────────────────────────────────────

LinearAllocator::LinearAllocator(u64 capacityBytes)
    : m_capacity(capacityBytes)
{
    m_buffer = static_cast<u8*>(GeneralAllocator::get().alloc(capacityBytes, 64));
    assert(m_buffer && "LinearAllocator: failed to allocate backing buffer");
}

LinearAllocator::~LinearAllocator() {
    GeneralAllocator::get().free(m_buffer);
}

void* LinearAllocator::alloc(u64 bytes, u32 alignment) {
    u64 aligned = (m_offset + alignment - 1) & ~static_cast<u64>(alignment - 1);
    if (aligned + bytes > m_capacity) return nullptr; // OOM
    m_offset = aligned + bytes;
    return m_buffer + aligned;
}

void LinearAllocator::reset() {
    m_offset = 0;
}

} // namespace shooter
