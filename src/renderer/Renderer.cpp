#include "renderer/Renderer.h"
#include "renderer/VulkanRenderer.h"
#include "core/Logger.h"

namespace shooter {

std::unique_ptr<Renderer> Renderer::create(const RendererDesc& desc) {
    switch (desc.backend) {
        case RendererBackend::Vulkan: {
#ifdef SHOOTER_VULKAN_AVAILABLE
            auto r = std::make_unique<VulkanRenderer>();
            if (!r->init(desc)) {
                LOG_WARN("Renderer", "Vulkan init failed – falling back to Stub");
                auto stub = std::make_unique<StubRenderer>();
                stub->init(desc);
                return stub;
            }
            return r;
#else
            LOG_INFO("Renderer", "Vulkan not compiled in – using Stub renderer");
            auto stub = std::make_unique<StubRenderer>();
            stub->init(desc);
            return stub;
#endif
        }
        case RendererBackend::Stub:
        default: {
            auto stub = std::make_unique<StubRenderer>();
            stub->init(desc);
            return stub;
        }
    }
}

} // namespace shooter
