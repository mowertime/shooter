#include "renderer/VulkanRenderer.h"
#include "core/Logger.h"

namespace shooter {

// ── Lifecycle ─────────────────────────────────────────────────────────────────

bool VulkanRenderer::init(const RendererDesc& desc) {
    m_desc = desc;
    LOG_INFO("VulkanRenderer", "Initialising Vulkan renderer");

#ifndef SHOOTER_VULKAN_AVAILABLE
    LOG_ERROR("VulkanRenderer", "Vulkan SDK not available at compile time");
    return false;
#else
    // In a full implementation this block would:
    //   1. Create VkInstance with required extensions.
    //   2. Select a physical device and queue families.
    //   3. Create logical device + graphics/compute/transfer queues.
    //   4. Create a VkSurfaceKHR from the OS window handle.
    //   5. Create VkSwapchainKHR.
    //   6. Allocate per-frame command buffers and synchronisation objects.
    //   7. Build the render pass graph (G-buffer, lighting, post-process).
    //   8. Compile / cache pipeline state objects.
    //   9. Optionally enable VK_KHR_ray_tracing_pipeline.

    m_raytracingEnabled = desc.enableRaytracing;
    m_initialised       = true;
    LOG_INFO("VulkanRenderer",
             "Vulkan renderer ready (raytracing=%s)",
             m_raytracingEnabled ? "on" : "off");
    return true;
#endif
}

void VulkanRenderer::shutdown() {
    if (!m_initialised) return;
    LOG_INFO("VulkanRenderer", "Shutting down Vulkan renderer");
    // Release swapchain, device, instance in reverse-creation order.
    m_initialised = false;
}

VulkanRenderer::~VulkanRenderer() {
    shutdown();
}

// ── Per-frame ─────────────────────────────────────────────────────────────────

bool VulkanRenderer::beginFrame() {
    if (!m_initialised) return false;
    // Acquire next swapchain image, wait for previous frame fence.
    m_frameIndex = (m_frameIndex + 1) % 2; // double-buffered
    return true;
}

void VulkanRenderer::endFrame() {
    if (!m_initialised) return;
    // Submit command buffer, present.
    m_stats.drawCalls = 0; // Reset per-frame counter.
}

void VulkanRenderer::resize(u32 width, u32 height) {
    LOG_INFO("VulkanRenderer", "Resizing swap-chain to %ux%u", width, height);
    // Recreate swapchain and framebuffers.
}

} // namespace shooter
