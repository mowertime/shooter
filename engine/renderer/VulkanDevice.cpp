// ---------------------------------------------------------------------------
// VulkanDevice.cpp — Vulkan RenderDevice implementation.
//
// When VULKAN_AVAILABLE is NOT defined the class is a complete logging stub
// so the project builds without the Vulkan SDK installed.
// When VULKAN_AVAILABLE IS defined the skeleton below can be fleshed out
// with actual Vulkan API calls.
// ---------------------------------------------------------------------------

#include "engine/renderer/VulkanDevice.h"
#include <iostream>

namespace shooter {

bool VulkanDevice::initialize(void* nativeWindowHandle, u32 width, u32 height) {
    m_width  = width;
    m_height = height;

#ifdef VULKAN_AVAILABLE
    std::cout << "[VulkanDevice] Initializing with Vulkan SDK\n";

    if (!createInstance())       return false;
    if (!selectPhysicalDevice()) return false;
    if (!createLogicalDevice())  return false;
    if (!createSwapchain(nativeWindowHandle, width, height)) return false;
    if (!createCommandPool())    return false;

    std::cout << "[VulkanDevice] Initialization complete ("
              << width << "x" << height << ")\n";
#else
    std::cout << "[VulkanDevice] Vulkan SDK not available — running in stub mode ("
              << width << "x" << height << ")\n";
    (void)nativeWindowHandle;
#endif

    m_initialized = true;
    return true;
}

void VulkanDevice::shutdown() {
    if (!m_initialized) return;

#ifdef VULKAN_AVAILABLE
    // Destroy in reverse creation order.
    // TODO: vkDestroyCommandPool, vkDestroySwapchainKHR, vkDestroyDevice, etc.
    std::cout << "[VulkanDevice] Shutting down Vulkan resources\n";
#else
    std::cout << "[VulkanDevice] Shutdown (stub)\n";
#endif

    m_initialized = false;
}

void VulkanDevice::beginFrame() {
#ifdef VULKAN_AVAILABLE
    // TODO: vkAcquireNextImageKHR, reset command buffer, begin render pass
#endif
}

void VulkanDevice::endFrame() {
#ifdef VULKAN_AVAILABLE
    // TODO: end render pass, submit command buffer, vkQueuePresentKHR
#endif
}

// ---------------------------------------------------------------------------
// Resource creation — returns incrementing handles so the rest of the engine
// can track resources regardless of whether real GPU objects exist.
// ---------------------------------------------------------------------------

BufferHandle VulkanDevice::createBuffer(const BufferDesc& desc) {
#ifdef VULKAN_AVAILABLE
    // TODO: vkCreateBuffer + vmaCreateAllocation or vkAllocateMemory
    std::cout << "[VulkanDevice] createBuffer " << desc.sizeBytes << " bytes\n";
#else
    (void)desc;
#endif
    return static_cast<BufferHandle>(m_nextBufferID++);
}

TextureHandle VulkanDevice::createTexture(const TextureDesc& desc) {
#ifdef VULKAN_AVAILABLE
    // TODO: vkCreateImage, vkCreateImageView, vkCreateSampler
    std::cout << "[VulkanDevice] createTexture "
              << desc.width << "x" << desc.height << "\n";
#else
    (void)desc;
#endif
    return static_cast<TextureHandle>(m_nextTextureID++);
}

ShaderHandle VulkanDevice::createShader(const ShaderDesc& desc) {
#ifdef VULKAN_AVAILABLE
    // TODO: vkCreateShaderModule from desc.spirvBytecode
    std::cout << "[VulkanDevice] createShader ("
              << desc.spirvBytecode.size() << " bytes SPIR-V)\n";
#else
    (void)desc;
#endif
    return static_cast<ShaderHandle>(m_nextShaderID++);
}

PipelineHandle VulkanDevice::createPipeline(const PipelineDesc& desc) {
#ifdef VULKAN_AVAILABLE
    // TODO: vkCreateGraphicsPipeline / vkCreateComputePipeline
    std::cout << "[VulkanDevice] createPipeline\n";
#else
    (void)desc;
#endif
    return static_cast<PipelineHandle>(m_nextPipelineID++);
}

void VulkanDevice::destroyBuffer(BufferHandle handle) {
#ifdef VULKAN_AVAILABLE
    // TODO: vkDestroyBuffer + free VMA allocation
#endif
    (void)handle;
}

void VulkanDevice::destroyTexture(TextureHandle handle) {
#ifdef VULKAN_AVAILABLE
    // TODO: vkDestroyImage, vkDestroyImageView, vkDestroySampler
#endif
    (void)handle;
}

void VulkanDevice::destroyShader(ShaderHandle handle) {
#ifdef VULKAN_AVAILABLE
    // TODO: vkDestroyShaderModule
#endif
    (void)handle;
}

void VulkanDevice::destroyPipeline(PipelineHandle handle) {
#ifdef VULKAN_AVAILABLE
    // TODO: vkDestroyPipeline
#endif
    (void)handle;
}

// ---------------------------------------------------------------------------
// Private helpers (Vulkan SDK only)
// ---------------------------------------------------------------------------
#ifdef VULKAN_AVAILABLE

bool VulkanDevice::createInstance() {
    // TODO: populate VkApplicationInfo, VkInstanceCreateInfo;
    // enable validation layers in debug; call vkCreateInstance.
    std::cout << "[VulkanDevice] createInstance\n";
    return true;
}

bool VulkanDevice::selectPhysicalDevice() {
    // TODO: vkEnumeratePhysicalDevices, score devices by VRAM / features,
    // pick the discrete GPU if available.
    std::cout << "[VulkanDevice] selectPhysicalDevice\n";
    return true;
}

bool VulkanDevice::createLogicalDevice() {
    // TODO: vkCreateDevice with graphics + compute + transfer queue families.
    std::cout << "[VulkanDevice] createLogicalDevice\n";
    return true;
}

bool VulkanDevice::createSwapchain(void* nativeHandle, u32 width, u32 height) {
    (void)nativeHandle; (void)width; (void)height;
    // TODO: create VkSurfaceKHR (platform-specific), query surface capabilities,
    // pick format / present mode, call vkCreateSwapchainKHR.
    std::cout << "[VulkanDevice] createSwapchain\n";
    return true;
}

bool VulkanDevice::createCommandPool() {
    // TODO: vkCreateCommandPool for the graphics queue family.
    std::cout << "[VulkanDevice] createCommandPool\n";
    return true;
}

#endif // VULKAN_AVAILABLE

} // namespace shooter
