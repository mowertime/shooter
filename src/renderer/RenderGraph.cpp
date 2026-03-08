#include "renderer/RenderGraph.h"
#include "core/Logger.h"

namespace shooter {

u32 RenderGraph::addPass(RenderPassDesc desc, PassCallback callback) {
    u32 idx = static_cast<u32>(m_passes.size());
    m_passes.push_back({std::move(desc), std::move(callback), {}});
    m_compiled = false;
    return idx;
}

void RenderGraph::compile() {
    LOG_DEBUG("RenderGraph", "Compiling render graph (%zu passes)",
              m_passes.size());
    // Topological sort and barrier insertion would happen here.
    m_compiled = true;
}

void RenderGraph::execute() {
    if (!m_compiled) compile();
    for (auto& pass : m_passes) {
        if (pass.desc.enabled && pass.callback) {
            pass.callback();
        }
    }
}

void RenderGraph::setPassEnabled(u32 passIndex, bool enabled) {
    if (passIndex < m_passes.size())
        m_passes[passIndex].desc.enabled = enabled;
}

} // namespace shooter
