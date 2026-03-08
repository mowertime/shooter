#pragma once

#include "core/Platform.h"
#include "core/Window.h"
#include <memory>
#include <string>

namespace shooter {

/// Supported rendering backends.
enum class RendererBackend : u8 {
    Vulkan    = 0,
    DirectX12 = 1,
    Stub      = 2, ///< Headless / test mode
};

/// Renderer initialisation parameters.
struct RendererDesc {
    RendererBackend backend{RendererBackend::Vulkan};
    Window*         window{nullptr};
    bool            enableValidation{false}; ///< Vulkan validation layers
    bool            enableRaytracing{false};
    bool            enableHDR{true};
    u32             shadowMapSize{4096};
    u32             msaaSamples{1};
};

/// Statistics reported each frame.
struct RendererStats {
    u32 drawCalls{0};
    u32 trianglesRendered{0};
    u32 gpuMemoryUsedMb{0};
    f32 gpuFrameTimeMs{0.0f};
};

/// Abstract renderer interface.
///
/// Concrete backends (Vulkan, DX12) derive from this class and are
/// selected at runtime.  The interface is deliberately minimal so that
/// upper-layer code remains backend-agnostic.
class Renderer {
public:
    /// Factory: allocates the correct backend implementation.
    static std::unique_ptr<Renderer> create(const RendererDesc& desc);

    virtual ~Renderer() = default;

    virtual bool init   (const RendererDesc& desc) = 0;
    virtual void shutdown() = 0;

    /// Begin a new frame; returns false if the swap-chain is out of date.
    virtual bool beginFrame() = 0;

    /// Submit all queued draw calls and present.
    virtual void endFrame()   = 0;

    /// Resize swap-chain (called on window resize).
    virtual void resize(u32 width, u32 height) = 0;

    virtual RendererStats getStats() const = 0;
    virtual const char*   backendName() const = 0;
};

} // namespace shooter
