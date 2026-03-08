#pragma once

// ---------------------------------------------------------------------------
// Memory.h — Custom allocators for data-oriented, cache-friendly allocation.
//
// Design philosophy:
//   • LinearAllocator  — fastest possible bump-pointer; reset by moving a ptr.
//   • StackAllocator   — supports LIFO deallocation via saved markers.
//   • PoolAllocator<T> — fixed-size blocks; eliminates fragmentation for ECS
//     component arrays, particle pools, etc.
// ---------------------------------------------------------------------------

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <atomic>
#include <new>

#include "Platform.h"

namespace shooter {

// ---------------------------------------------------------------------------
// AllocationStats — lightweight tracker shared across allocators.
// ---------------------------------------------------------------------------
struct AllocationStats {
    std::atomic<u64> totalBytesAllocated{0};
    std::atomic<u64> currentBytesInUse{0};
    std::atomic<u64> allocationCount{0};
    std::atomic<u64> freeCount{0};

    void recordAlloc(u64 bytes) {
        totalBytesAllocated.fetch_add(bytes, std::memory_order_relaxed);
        currentBytesInUse.fetch_add(bytes, std::memory_order_relaxed);
        allocationCount.fetch_add(1, std::memory_order_relaxed);
    }
    void recordFree(u64 bytes) {
        currentBytesInUse.fetch_sub(bytes, std::memory_order_relaxed);
        freeCount.fetch_add(1, std::memory_order_relaxed);
    }
};

// ---------------------------------------------------------------------------
// LinearAllocator — O(1) allocate, O(1) reset.
// Not thread-safe by itself; intended for per-frame, per-thread scratch memory.
// ---------------------------------------------------------------------------
class LinearAllocator {
public:
    explicit LinearAllocator(u64 capacityBytes) {
        m_buffer   = static_cast<u8*>(std::malloc(capacityBytes));
        m_capacity = capacityBytes;
        m_offset   = 0;
        SE_ASSERT(m_buffer != nullptr, "LinearAllocator: malloc failed");
    }

    ~LinearAllocator() {
        std::free(m_buffer);
    }

    // Non-copyable
    LinearAllocator(const LinearAllocator&)            = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;

    /// Allocate 'bytes' aligned to 'alignment'. Returns nullptr if OOM.
    [[nodiscard]] void* allocate(u64 bytes, u64 alignment = alignof(std::max_align_t)) {
        u64 aligned = (m_offset + alignment - 1) & ~(alignment - 1);
        if (aligned + bytes > m_capacity) return nullptr;
        void* ptr  = m_buffer + aligned;
        m_offset   = aligned + bytes;
        m_stats.recordAlloc(bytes);
        return ptr;
    }

    /// Allocate and default-construct T.
    template<typename T>
    [[nodiscard]] T* allocate() {
        void* mem = allocate(sizeof(T), alignof(T));
        return mem ? new(mem) T{} : nullptr;
    }

    /// Rewind the allocator — all previous pointers become invalid.
    void reset() {
        m_stats.recordFree(m_offset);
        m_offset = 0;
    }

    [[nodiscard]] u64 usedBytes()  const { return m_offset; }
    [[nodiscard]] u64 totalBytes() const { return m_capacity; }
    [[nodiscard]] const AllocationStats& stats() const { return m_stats; }

private:
    u8*              m_buffer{nullptr};
    u64              m_capacity{0};
    u64              m_offset{0};
    AllocationStats  m_stats;
};

// ---------------------------------------------------------------------------
// StackAllocator — like LinearAllocator but supports LIFO deallocation.
// Saves a 'Marker' (current offset) which can be restored later.
// ---------------------------------------------------------------------------
class StackAllocator {
public:
    using Marker = u64; ///< Opaque offset bookmark.

    explicit StackAllocator(u64 capacityBytes) {
        m_buffer   = static_cast<u8*>(std::malloc(capacityBytes));
        m_capacity = capacityBytes;
        m_offset   = 0;
        SE_ASSERT(m_buffer != nullptr, "StackAllocator: malloc failed");
    }

    ~StackAllocator() { std::free(m_buffer); }

    StackAllocator(const StackAllocator&)            = delete;
    StackAllocator& operator=(const StackAllocator&) = delete;

    [[nodiscard]] void* allocate(u64 bytes, u64 alignment = alignof(std::max_align_t)) {
        u64 aligned = (m_offset + alignment - 1) & ~(alignment - 1);
        if (aligned + bytes > m_capacity) return nullptr;
        void* ptr  = m_buffer + aligned;
        m_offset   = aligned + bytes;
        m_stats.recordAlloc(bytes);
        return ptr;
    }

    /// Save the current stack top.
    [[nodiscard]] Marker getMarker() const { return m_offset; }

    /// Restore to a previously saved marker — frees everything allocated after it.
    void freeToMarker(Marker marker) {
        SE_ASSERT(marker <= m_offset, "Invalid stack marker");
        m_stats.recordFree(m_offset - marker);
        m_offset = marker;
    }

    void reset() { freeToMarker(0); }

    [[nodiscard]] u64 usedBytes()  const { return m_offset; }
    [[nodiscard]] const AllocationStats& stats() const { return m_stats; }

private:
    u8*             m_buffer{nullptr};
    u64             m_capacity{0};
    u64             m_offset{0};
    AllocationStats m_stats;
};

// ---------------------------------------------------------------------------
// PoolAllocator<T, BlockSize> — fixed-size-block pool for type T.
// Extremely fast for ECS component pools: allocate/free are O(1) and the
// free list is embedded inside the freed blocks (no extra storage needed).
// ---------------------------------------------------------------------------
template<typename T, u32 BlockSize = 64>
class PoolAllocator {
    static_assert(sizeof(T) >= sizeof(void*),
                  "PoolAllocator: T must be at least pointer-sized");
public:
    PoolAllocator() { growPool(); }

    ~PoolAllocator() {
        for (auto* block : m_blocks) {
            std::free(block);
        }
    }

    PoolAllocator(const PoolAllocator&)            = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    [[nodiscard]] T* allocate() {
        if (!m_freeHead) growPool();
        FreeNode* node = m_freeHead;
        m_freeHead     = node->next;
        m_stats.recordAlloc(sizeof(T));
        return reinterpret_cast<T*>(node);
    }

    void deallocate(T* ptr) {
        if (!ptr) return;
        auto* node = reinterpret_cast<FreeNode*>(ptr);
        node->next = m_freeHead;
        m_freeHead = node;
        m_stats.recordFree(sizeof(T));
    }

    [[nodiscard]] const AllocationStats& stats() const { return m_stats; }

private:
    struct FreeNode { FreeNode* next; };

    void growPool() {
        // Allocate one contiguous block of BlockSize items.
        auto* raw = static_cast<u8*>(std::malloc(sizeof(T) * BlockSize));
        SE_ASSERT(raw != nullptr, "PoolAllocator: malloc failed");
        m_blocks.push_back(raw);
        // Thread the new block onto the free list.
        for (u32 i = 0; i < BlockSize; ++i) {
            auto* node = reinterpret_cast<FreeNode*>(raw + i * sizeof(T));
            node->next = m_freeHead;
            m_freeHead = node;
        }
    }

    FreeNode*          m_freeHead{nullptr};
    std::vector<u8*>   m_blocks;
    AllocationStats    m_stats;
};

} // namespace shooter
