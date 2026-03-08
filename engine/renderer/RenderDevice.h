#pragma once

// ---------------------------------------------------------------------------
// RenderDevice.h — Low-level GPU device abstraction.
//
// This is the boundary between the engine's renderer and the underlying
// graphics API (Vulkan, DirectX 12, Metal, …).  All API-specific code lives
// in concrete subclasses; the rest of the engine only sees RenderDevice.
// ---------------------------------------------------------------------------

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

#include "engine/platform/Platform.h"

namespace shooter {

// ---- Resource handles -----------------------------------------------------
// Opaque 64-bit handles so the engine never holds raw GPU pointers.
enum class BufferHandle  : u64 { Invalid = 0 };
enum class TextureHandle : u64 { Invalid = 0 };
enum class ShaderHandle  : u64 { Invalid = 0 };
enum class PipelineHandle: u64 { Invalid = 0 };

// ---- Buffer descriptor -----------------------------------------------------
enum class BufferUsage : u32 {
    Vertex   = 1 << 0,
    Index    = 1 << 1,
    Uniform  = 1 << 2,
    Storage  = 1 << 3,
    Staging  = 1 << 4,
};

struct BufferDesc {
    u64         sizeBytes{0};
    BufferUsage usage{BufferUsage::Vertex};
    const void* initialData{nullptr};  ///< nullptr = leave uninitialised
};

// ---- Texture descriptor ----------------------------------------------------
enum class TextureFormat { RGBA8_UNORM, RGBA16_FLOAT, R32_FLOAT, Depth32 };

struct TextureDesc {
    u32           width{1};
    u32           height{1};
    u32           depth{1};       ///< 1 = 2D, >1 = 3D / array
    u32           mipLevels{1};
    TextureFormat format{TextureFormat::RGBA8_UNORM};
    const void*   initialData{nullptr};
};

// ---- Shader descriptor -----------------------------------------------------
enum class ShaderStage { Vertex, Fragment, Compute, Geometry };

struct ShaderDesc {
    ShaderStage            stage;
    std::vector<uint8_t>   spirvBytecode;  ///< SPIR-V for Vulkan; DXIL for D3D12
    std::string            entryPoint{"main"};
};

// ---- Pipeline descriptor ---------------------------------------------------
struct PipelineDesc {
    ShaderHandle vertexShader{ShaderHandle::Invalid};
    ShaderHandle fragmentShader{ShaderHandle::Invalid};
    ShaderHandle computeShader{ShaderHandle::Invalid};
    bool         depthTest{true};
    bool         depthWrite{true};
    bool         blendEnabled{false};
    // TODO: vertex layout, render pass, descriptor set layouts, …
};

// ===========================================================================
// RenderDevice — abstract base class.
// ===========================================================================
class RenderDevice {
public:
    virtual ~RenderDevice() = default;

    // ---- Lifecycle ---------------------------------------------------------
    virtual bool initialize(void* nativeWindowHandle,
                            u32 width, u32 height) = 0;
    virtual void shutdown() = 0;

    // ---- Frame boundary ----------------------------------------------------
    virtual void beginFrame() = 0;
    virtual void endFrame()   = 0;

    // ---- Resource creation -------------------------------------------------
    virtual BufferHandle   createBuffer(const BufferDesc& desc)     = 0;
    virtual TextureHandle  createTexture(const TextureDesc& desc)   = 0;
    virtual ShaderHandle   createShader(const ShaderDesc& desc)     = 0;
    virtual PipelineHandle createPipeline(const PipelineDesc& desc) = 0;

    // ---- Resource destruction ----------------------------------------------
    virtual void destroyBuffer(BufferHandle handle)     = 0;
    virtual void destroyTexture(TextureHandle handle)   = 0;
    virtual void destroyShader(ShaderHandle handle)     = 0;
    virtual void destroyPipeline(PipelineHandle handle) = 0;

    // ---- Queries -----------------------------------------------------------
    [[nodiscard]] virtual u32 getWidth()  const = 0;
    [[nodiscard]] virtual u32 getHeight() const = 0;
    [[nodiscard]] virtual const char* apiName() const = 0;
};

} // namespace shooter
