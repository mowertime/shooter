#include "ai/PerceptionSystem.h"
#include <algorithm>

namespace shooter {

u32 PerceptionSystem::registerAgent(u32 entityId) {
    u32 idx = static_cast<u32>(m_agents.size());
    m_agents.push_back({entityId, {}});
    return idx;
}

void PerceptionSystem::unregisterAgent(u32 entityId) {
    m_agents.erase(
        std::remove_if(m_agents.begin(), m_agents.end(),
                       [entityId](const AgentEntry& a) {
                           return a.entityId == entityId;
                       }),
        m_agents.end());
}

void PerceptionSystem::broadcastStimulus(const PerceptionEvent& event,
                                          float /*radius*/,
                                          float gameTime) {
    for (auto& a : m_agents) {
        if (a.entityId == event.sourceEntityId) continue;
        if (event.type == StimulusType::Visual) {
            a.state.canSeeEnemy      = true;
            a.state.primaryThreatId  = event.sourceEntityId;
            a.state.lastSeenEnemyX   = event.px;
            a.state.lastSeenEnemyY   = event.py;
            a.state.lastSeenEnemyZ   = event.pz;
            a.state.lastSeenTime     = gameTime;
        } else {
            a.state.lastHeardTime = gameTime;
        }
    }
}

void PerceptionSystem::updateSight(float gameTime) {
    // Decay visual contacts after 10 seconds without refresh.
    for (auto& a : m_agents) {
        if (a.state.canSeeEnemy &&
            (gameTime - a.state.lastSeenTime) > 10.0f) {
            a.state.canSeeEnemy = false;
        }
    }
}

const PerceptionState* PerceptionSystem::getState(u32 entityId) const {
    for (const auto& a : m_agents) {
        if (a.entityId == entityId) return &a.state;
    }
    return nullptr;
}

} // namespace shooter
