#pragma once

#include "renderer/Renderer.h"

namespace shooter {

/// Vulkan renderer backend.
///
/// Implements a deferred + forward hybrid pipeline with PBR materials,
/// volumetric lighting, atmospheric scattering, HDR tone-mapping, and
/// optional ray-traced global illumination.
class VulkanRenderer final : public Renderer {
public:
    VulkanRenderer() = default;
    ~VulkanRenderer() override;

    bool init   (const RendererDesc& desc) override;
    void shutdown() override;
    bool beginFrame() override;
    void endFrame()   override;
    void resize(u32 width, u32 height) override;

    RendererStats getStats()      const override { return m_stats; }
    const char*   backendName()   const override { return "Vulkan"; }

private:
    // ── Vulkan handles (opaque in this stub) ──────────────────────────────────
    void* m_instance{nullptr};       ///< VkInstance
    void* m_physicalDevice{nullptr}; ///< VkPhysicalDevice
    void* m_device{nullptr};         ///< VkDevice
    void* m_surface{nullptr};        ///< VkSurfaceKHR
    void* m_swapchain{nullptr};      ///< VkSwapchainKHR
    void* m_graphicsQueue{nullptr};  ///< VkQueue

    u32  m_frameIndex{0};
    bool m_initialised{false};
    bool m_raytracingEnabled{false};

    RendererDesc  m_desc{};
    RendererStats m_stats{};
};

/// Stub renderer used when no GPU API is available (unit tests, CI).
class StubRenderer final : public Renderer {
public:
    bool init   (const RendererDesc&) override { return true; }
    void shutdown() override {}
    bool beginFrame() override { return true; }
    void endFrame()   override { ++m_stats.drawCalls; }
    void resize(u32, u32) override {}

    RendererStats getStats()    const override { return m_stats; }
    const char*   backendName() const override { return "Stub";  }

private:
    RendererStats m_stats{};
};

} // namespace shooter
