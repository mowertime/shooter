#include "renderer/GpuBuffer.h"
#include "core/Memory.h"
#include "core/Logger.h"

namespace shooter {

bool GpuBuffer::create(u64 sizeBytes, GpuBufferUsage usage) {
    m_size  = sizeBytes;
    m_usage = usage;
    // In a Vulkan build: vmaCreateBuffer(…)
    // For the stub, allocate a plain CPU buffer so tests can run headlessly.
    m_handle = GeneralAllocator::get().alloc(sizeBytes, 16);
    if (!m_handle) {
        LOG_ERROR("GpuBuffer", "Failed to allocate buffer of %llu bytes",
                  static_cast<unsigned long long>(sizeBytes));
        return false;
    }
    return true;
}

bool GpuBuffer::upload(const void* data, u64 sizeBytes, u64 offsetBytes) {
    if (!m_handle) return false;
    if (offsetBytes + sizeBytes > m_size) return false;
    std::memcpy(static_cast<u8*>(m_handle) + offsetBytes, data,
                static_cast<size_t>(sizeBytes));
    return true;
}

void* GpuBuffer::map() { return m_handle; }
void  GpuBuffer::unmap() {}

void GpuBuffer::destroy() {
    if (m_handle) {
        GeneralAllocator::get().free(m_handle);
        m_handle = nullptr;
        m_size   = 0;
    }
}

GpuBuffer::~GpuBuffer() { destroy(); }

} // namespace shooter
