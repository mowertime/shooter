#pragma once

#include "core/Platform.h"
#include <array>
#include <atomic>
#include <cstring>
#include <new>

namespace shooter {

/// Alignment-aware general allocator.
///
/// All engine subsystems obtain memory through an allocator so that
/// profiling and leak detection can be centralised.
class GeneralAllocator {
public:
    static GeneralAllocator& get();

    void* alloc  (u64 bytes, u32 alignment = 16);
    void  free   (void* ptr);
    void* realloc(void* ptr, u64 newBytes, u32 alignment = 16);

    u64   bytesAllocated() const;
    u64   allocationCount() const;
};

/// Linear (bump-pointer) allocator.  Used for per-frame temporary data.
/// Entire arena is reset at end of frame via \c reset().
class LinearAllocator {
public:
    explicit LinearAllocator(u64 capacityBytes);
    ~LinearAllocator();

    void* alloc(u64 bytes, u32 alignment = 16);
    void  reset();

    u64   capacity() const { return m_capacity; }
    u64   used()     const { return m_offset;   }

private:
    u8*  m_buffer{nullptr};
    u64  m_capacity{0};
    u64  m_offset{0};
};

/// Pool allocator for fixed-size objects (e.g. ECS components).
template<u64 ObjectSize, u32 MaxObjects>
class PoolAllocator {
public:
    PoolAllocator() {
        // Build the free list.
        m_freeHead = 0;
        for (u32 i = 0; i < MaxObjects - 1; ++i)
            *reinterpret_cast<u32*>(&m_storage[i * ObjectSize]) = i + 1;
        *reinterpret_cast<u32*>(&m_storage[(MaxObjects - 1) * ObjectSize]) =
            UINT32_MAX; // sentinel
    }

    void* alloc() {
        if (m_freeHead == UINT32_MAX) return nullptr;
        u8*  ptr      = &m_storage[m_freeHead * ObjectSize];
        m_freeHead    = *reinterpret_cast<u32*>(ptr);
        ++m_used;
        return ptr;
    }

    void free(void* ptr) {
        u32 idx = static_cast<u32>(
            (static_cast<u8*>(ptr) - m_storage.data()) / ObjectSize);
        *reinterpret_cast<u32*>(ptr) = m_freeHead;
        m_freeHead = idx;
        --m_used;
    }

    u32 used()     const { return m_used;       }
    u32 capacity() const { return MaxObjects;   }

private:
    alignas(64) std::array<u8, ObjectSize * MaxObjects> m_storage{};
    u32 m_freeHead{0};
    u32 m_used{0};
};

// ── Convenience macros ────────────────────────────────────────────────────────
#define SHOOTER_NEW(T, ...)   new (GeneralAllocator::get().alloc(sizeof(T))) T(__VA_ARGS__)
#define SHOOTER_DELETE(T, p)  do { if (p) { (p)->~T(); GeneralAllocator::get().free(p); (p) = nullptr; } } while(0)

} // namespace shooter
