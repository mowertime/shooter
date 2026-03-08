#pragma once

#include "core/Platform.h"

namespace shooter {

/// GPU memory usage type.
enum class GpuBufferUsage : u8 {
    Vertex,
    Index,
    Uniform,
    Storage,
    Staging,
};

/// Abstract GPU buffer (vertex, index, uniform, storage).
///
/// In a Vulkan build this wraps a VkBuffer + VmaAllocation.
class GpuBuffer {
public:
    GpuBuffer() = default;
    ~GpuBuffer();

    /// Allocate GPU memory.  Returns false on failure.
    bool create(u64 sizeBytes, GpuBufferUsage usage);

    /// Upload CPU data to this buffer (synchronous, for small buffers).
    bool upload(const void* data, u64 sizeBytes, u64 offsetBytes = 0);

    /// Map the buffer for CPU access (host-visible buffers only).
    void* map();
    void  unmap();

    void   destroy();
    u64    size()    const { return m_size;    }
    bool   isValid() const { return m_handle != nullptr; }

private:
    void* m_handle{nullptr}; ///< Backend-specific buffer handle
    void* m_allocation{nullptr};
    u64   m_size{0};
    GpuBufferUsage m_usage{GpuBufferUsage::Vertex};
};

} // namespace shooter
