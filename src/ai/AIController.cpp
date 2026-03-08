#include "ai/AIController.h"

namespace shooter {

AIController::AIController(u32 entityId, u8 factionId)
    : m_entityId(entityId)
    , m_factionId(factionId)
    , m_tree(buildInfantryBT())
{}

void AIController::update(float dt) {
    if (m_tree) {
        m_lastStatus = m_tree->tick(m_bb, dt);
    }
}

// ── AIControllerPool ──────────────────────────────────────────────────────────

u32 AIControllerPool::create(u32 entityId, u8 factionId) {
    u32 id = static_cast<u32>(m_controllers.size());
    m_controllers.push_back(
        std::make_unique<AIController>(entityId, factionId));
    return id;
}

void AIControllerPool::destroy(u32 id) {
    if (id < m_controllers.size()) {
        m_controllers[id].reset();
    }
}

AIController* AIControllerPool::get(u32 id) {
    if (id >= m_controllers.size()) return nullptr;
    return m_controllers[id].get();
}

void AIControllerPool::updateAll(float dt) {
    for (auto& c : m_controllers) {
        if (c) c->update(dt);
    }
}

u32 AIControllerPool::count() const {
    u32 n = 0;
    for (const auto& c : m_controllers) n += c ? 1 : 0;
    return n;
}

} // namespace shooter
