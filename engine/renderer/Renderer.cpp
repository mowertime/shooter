// ---------------------------------------------------------------------------
// Renderer.cpp — High-level renderer implementation.
// ---------------------------------------------------------------------------

#include "engine/renderer/Renderer.h"
#include "engine/renderer/VulkanDevice.h"

#include <iostream>
#include <algorithm>

namespace shooter {

bool Renderer::initialize(const RendererDesc& desc) {
    m_desc = desc;

    // Select and create the underlying API device.
    switch (desc.api) {
        case RendererAPI::Vulkan:
            m_device = std::make_unique<VulkanDevice>();
            break;

        case RendererAPI::DirectX12:
            // TODO: create D3D12Device
            std::cerr << "[Renderer] DirectX12 not yet implemented, falling back to stub\n";
            m_device = std::make_unique<VulkanDevice>();
            break;

        case RendererAPI::Stub:
        default:
            std::cout << "[Renderer] Using stub renderer\n";
            m_device = std::make_unique<VulkanDevice>(); // runs as stub without SDK
            break;
    }

    if (!m_device->initialize(desc.nativeWindowHandle, desc.width, desc.height)) {
        std::cerr << "[Renderer] Failed to initialize render device\n";
        return false;
    }

    std::cout << "[Renderer] Initialized ("
              << m_device->apiName() << " "
              << desc.width << "x" << desc.height << ")\n";
    return true;
}

void Renderer::shutdown() {
    if (m_device) {
        m_device->shutdown();
        m_device.reset();
    }
    std::cout << "[Renderer] Shutdown\n";
}

void Renderer::beginFrame() {
    m_meshQueue.clear();
    m_terrainQueue.clear();
    m_particleQueue.clear();
    if (m_device) m_device->beginFrame();
}

void Renderer::endFrame() {
    // --- 1. Shadow pass: render depth from each shadow-casting light --------
    executeShadowPass();

    // --- 2. G-buffer pass: write albedo/normal/roughness/metallic -----------
    executeGBufferPass();

    // --- 3. Deferred lighting pass ------------------------------------------
    executeLightingPass();

    // --- 4. Forward pass: transparent objects, particles --------------------
    executeForwardPass();

    // --- 5. Post-process: TAA, bloom, lens flare, colour grade --------------
    executePostProcess();

    if (m_device) m_device->endFrame();
}

// ---- Draw-call accumulation ------------------------------------------------

void Renderer::drawMesh(const MeshDrawCall& call) {
    m_meshQueue.push_back(call);
}

void Renderer::drawTerrain(const TerrainDrawCall& call) {
    m_terrainQueue.push_back(call);
}

void Renderer::drawParticles(const ParticleDrawCall& call) {
    m_particleQueue.push_back(call);
}

// ---- Resource forwarding ---------------------------------------------------

BufferHandle Renderer::createBuffer(const BufferDesc& desc) {
    SE_ASSERT(m_device, "Renderer not initialized");
    return m_device->createBuffer(desc);
}

TextureHandle Renderer::createTexture(const TextureDesc& desc) {
    SE_ASSERT(m_device, "Renderer not initialized");
    return m_device->createTexture(desc);
}

ShaderHandle Renderer::createShader(const ShaderDesc& desc) {
    SE_ASSERT(m_device, "Renderer not initialized");
    return m_device->createShader(desc);
}

PipelineHandle Renderer::createPipeline(const PipelineDesc& desc) {
    SE_ASSERT(m_device, "Renderer not initialized");
    return m_device->createPipeline(desc);
}

// ---- Private passes --------------------------------------------------------

void Renderer::executeShadowPass() {
    // TODO: bind shadow pipeline; for each shadow-casting mesh in m_meshQueue,
    // render depth from the light's perspective into a depth atlas.
}

void Renderer::executeGBufferPass() {
    // TODO: bind G-buffer render targets (MRT); submit m_terrainQueue first
    // (back-to-front is irrelevant for deferred), then opaque m_meshQueue.
    (void)m_terrainQueue;
    (void)m_meshQueue;
}

void Renderer::executeLightingPass() {
    // TODO: fullscreen triangle pass; sample G-buffer; evaluate PBR BRDF for
    // each light; apply shadow maps; add IBL diffuse + specular.
}

void Renderer::executeForwardPass() {
    // TODO: render transparent meshes (sorted back-to-front) and
    // m_particleQueue using additive / alpha blend.
    (void)m_particleQueue;
}

void Renderer::executePostProcess() {
    // TODO: TAA jitter resolve → bloom threshold/blur → lens distortion →
    // ACES tone-map → FXAA → UI composite.
}

} // namespace shooter
