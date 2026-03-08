#include "ai/SquadManager.h"
#include <cmath>
#include <algorithm>

namespace shooter {

// ── Squad ─────────────────────────────────────────────────────────────────────

Squad::Squad(u32 squadId, u8 factionId)
    : m_squadId(squadId), m_factionId(factionId) {}

void Squad::addMember(u32 controllerId) {
    if (m_memberIds.size() < MAX_MEMBERS)
        m_memberIds.push_back(controllerId);
}

void Squad::removeMember(u32 controllerId) {
    m_memberIds.erase(
        std::remove(m_memberIds.begin(), m_memberIds.end(), controllerId),
        m_memberIds.end());
}

void Squad::update(AIControllerPool& pool, float dt) {
    (void)dt;
    // Distribute formation positions around the squad objective.
    u32 n = static_cast<u32>(m_memberIds.size());
    for (u32 i = 0; i < n; ++i) {
        AIController* c = pool.get(m_memberIds[i]);
        if (!c) continue;

        float angle  = (2.0f * 3.14159265f / static_cast<float>(n)) * i;
        float radius = (m_formation == FormationType::Column) ? 3.0f * i : 5.0f;
        float fx     = m_objX + std::cos(angle) * radius;
        float fz     = m_objZ + std::sin(angle) * radius;

        c->blackboard().targetX = fx;
        c->blackboard().targetY = m_objY;
        c->blackboard().targetZ = fz;
        c->blackboard().squadId = m_squadId;
    }
}

u32 Squad::memberCount() const {
    return static_cast<u32>(m_memberIds.size());
}

// ── SquadManager ──────────────────────────────────────────────────────────────

u32 SquadManager::createSquad(u8 factionId) {
    u32 id = static_cast<u32>(m_squads.size());
    m_squads.emplace_back(id, factionId);
    return id;
}

Squad* SquadManager::getSquad(u32 id) {
    if (id >= m_squads.size()) return nullptr;
    return &m_squads[id];
}

void SquadManager::updateAll(AIControllerPool& pool, float dt) {
    for (auto& s : m_squads) s.update(pool, dt);
}

} // namespace shooter
