#pragma once

// ---------------------------------------------------------------------------
// Renderer.h — High-level renderer interface.
//
// Sits above RenderDevice and orchestrates passes: shadow maps, G-buffer,
// deferred lighting, forward-transparent, post-process, UI.
// ---------------------------------------------------------------------------

#include <memory>
#include <string>
#include <array>

#include "RenderDevice.h"
#include "engine/platform/Platform.h"

namespace shooter {

enum class RendererAPI { Vulkan, DirectX12, Stub };

struct RendererDesc {
    RendererAPI api{RendererAPI::Vulkan};
    void*       nativeWindowHandle{nullptr};
    u32         width{1920};
    u32         height{1080};
    bool        vsync{false};
    bool        enableValidation{true}; ///< API validation layers in debug
};

// ---- Mesh/terrain draw calls -----------------------------------------------
struct MeshDrawCall {
    BufferHandle  vertexBuffer{BufferHandle::Invalid};
    BufferHandle  indexBuffer{BufferHandle::Invalid};
    u32           indexCount{0};
    PipelineHandle pipeline{PipelineHandle::Invalid};
    // Transform passed via push constants / uniform buffer
    float         modelMatrix[16]{};
};

struct TerrainDrawCall {
    u32  chunkX{0};
    u32  chunkZ{0};
    u32  lodLevel{0};
    BufferHandle heightBuffer{BufferHandle::Invalid};
};

struct ParticleDrawCall {
    u32           particleCount{0};
    BufferHandle  particleBuffer{BufferHandle::Invalid};
    TextureHandle atlasTexture{TextureHandle::Invalid};
};

// ===========================================================================
// Renderer — high-level, API-agnostic renderer.
//
// Render pipeline overview (each frame):
//   1. Shadow pass   — render scene depth from sun/spot lights
//   2. G-buffer pass — deferred geometry: albedo, normal, roughness, metallic
//   3. Lighting pass — screen-space deferred + IBL
//   4. Forward pass  — transparent objects, particles, UI elements
//   5. Post-process  — TAA, bloom, lens-flare, colour grading
// ===========================================================================
class Renderer {
public:
    Renderer()  = default;
    ~Renderer() = default;

    // Non-copyable
    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool initialize(const RendererDesc& desc);
    void shutdown();

    // ---- Per-frame API -----------------------------------------------------
    void beginFrame();
    void endFrame();

    void drawMesh(const MeshDrawCall& call);
    void drawTerrain(const TerrainDrawCall& call);
    void drawParticles(const ParticleDrawCall& call);

    // ---- Resource forwarding helpers ---------------------------------------
    [[nodiscard]] BufferHandle   createBuffer(const BufferDesc& desc);
    [[nodiscard]] TextureHandle  createTexture(const TextureDesc& desc);
    [[nodiscard]] ShaderHandle   createShader(const ShaderDesc& desc);
    [[nodiscard]] PipelineHandle createPipeline(const PipelineDesc& desc);

    [[nodiscard]] RenderDevice* getDevice() const { return m_device.get(); }

private:
    std::unique_ptr<RenderDevice> m_device;
    RendererDesc                  m_desc;

    // Per-frame draw-call queues (sorted before submission)
    std::vector<MeshDrawCall>     m_meshQueue;
    std::vector<TerrainDrawCall>  m_terrainQueue;
    std::vector<ParticleDrawCall> m_particleQueue;

    void executeShadowPass();
    void executeGBufferPass();
    void executeLightingPass();
    void executeForwardPass();
    void executePostProcess();
};

} // namespace shooter
