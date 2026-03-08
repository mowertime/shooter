#pragma once

// ---------------------------------------------------------------------------
// VulkanDevice.h — Vulkan implementation of RenderDevice.
//
// When the Vulkan SDK is available (VULKAN_AVAILABLE defined by CMake)
// actual Vk* calls are made.  Otherwise this is a logging stub so the
// project compiles everywhere.
// ---------------------------------------------------------------------------

#include "RenderDevice.h"
#include <iostream>

#ifdef VULKAN_AVAILABLE
    #include <vulkan/vulkan.h>
#endif

namespace shooter {

class VulkanDevice : public RenderDevice {
public:
    VulkanDevice()  = default;
    ~VulkanDevice() override { shutdown(); }

    // ---- RenderDevice interface --------------------------------------------
    bool initialize(void* nativeWindowHandle, u32 width, u32 height) override;
    void shutdown() override;

    void beginFrame() override;
    void endFrame()   override;

    BufferHandle   createBuffer(const BufferDesc& desc)     override;
    TextureHandle  createTexture(const TextureDesc& desc)   override;
    ShaderHandle   createShader(const ShaderDesc& desc)     override;
    PipelineHandle createPipeline(const PipelineDesc& desc) override;

    void destroyBuffer(BufferHandle handle)     override;
    void destroyTexture(TextureHandle handle)   override;
    void destroyShader(ShaderHandle handle)     override;
    void destroyPipeline(PipelineHandle handle) override;

    u32         getWidth()  const override { return m_width; }
    u32         getHeight() const override { return m_height; }
    const char* apiName()   const override { return "Vulkan"; }

private:
    u32  m_width{0};
    u32  m_height{0};
    bool m_initialized{false};

    // Monotonically increasing counters used to produce unique handles.
    u64 m_nextBufferID{1};
    u64 m_nextTextureID{1};
    u64 m_nextShaderID{1};
    u64 m_nextPipelineID{1};

#ifdef VULKAN_AVAILABLE
    // Vulkan objects (allocated during initialize())
    VkInstance               m_instance{VK_NULL_HANDLE};
    VkPhysicalDevice         m_physicalDevice{VK_NULL_HANDLE};
    VkDevice                 m_device{VK_NULL_HANDLE};
    VkQueue                  m_graphicsQueue{VK_NULL_HANDLE};
    VkSurfaceKHR             m_surface{VK_NULL_HANDLE};
    VkSwapchainKHR           m_swapchain{VK_NULL_HANDLE};
    VkCommandPool            m_commandPool{VK_NULL_HANDLE};
    VkDebugUtilsMessengerEXT m_debugMessenger{VK_NULL_HANDLE};

    // Internal helpers
    bool createInstance();
    bool selectPhysicalDevice();
    bool createLogicalDevice();
    bool createSwapchain(void* nativeHandle, u32 width, u32 height);
    bool createCommandPool();
#endif
};

} // namespace shooter
