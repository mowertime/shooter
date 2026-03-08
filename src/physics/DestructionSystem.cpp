#include "physics/DestructionSystem.h"

namespace shooter {

u32 DestructionSystem::addStructure(std::vector<DestructibleChunk> chunks) {
    u32 id = static_cast<u32>(m_structures.size());
    m_structures.push_back({std::move(chunks), 0.0f});
    return id;
}

bool DestructionSystem::applyDamage(u32 structureId, u32 chunkId, float damage) {
    if (structureId >= m_structures.size()) return false;
    auto& s = m_structures[structureId];
    if (chunkId >= s.chunks.size()) return false;
    auto& c = s.chunks[chunkId];
    if (c.isDestroyed) return false;
    c.health -= damage;
    if (c.health <= 0.0f) {
        c.isDestroyed = true;
        c.health      = 0.0f;
        // A full implementation would fracture the mesh and spawn debris.
        return true;
    }
    return false;
}

void DestructionSystem::update(float dt) {
    for (auto& s : m_structures) {
        s.debrisTimer += dt;
        // Remove very old debris after 60 s to free physics bodies.
    }
}

u32 DestructionSystem::structureCount() const {
    return static_cast<u32>(m_structures.size());
}

} // namespace shooter
