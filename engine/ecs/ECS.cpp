// ---------------------------------------------------------------------------
// ECS.cpp — ECSWorld implementation.
// ---------------------------------------------------------------------------

#include "engine/ecs/ECS.h"
#include <stdexcept>
#include <iostream>

namespace shooter {

ECSWorld::ECSWorld() {
    // Reserve space to avoid frequent rehashing.
    m_entityRecords.reserve(MAX_ENTITIES);
    m_freeIDs.reserve(1024);
}

EntityID ECSWorld::createEntity() {
    EntityID id;
    if (!m_freeIDs.empty()) {
        id = m_freeIDs.back();
        m_freeIDs.pop_back();
    } else {
        id = m_nextEntityID++;
    }

    m_entityRecords[id] = EntityRecord{};
    ++m_aliveCount;
    return id;
}

void ECSWorld::destroyEntity(EntityID id) {
    if (!isValid(id)) return;

    auto& record = m_entityRecords[id];
    if (record.archetype) {
        auto it = record.archetype->entityIndex.find(id);
        if (it != record.archetype->entityIndex.end()) {
            u32 idx = it->second;
            // Swap-erase every component column.
            for (auto& [cid, col] : record.archetype->columns) {
                col->removeEntity(idx);
            }
            // Fix up the entity that was swapped into slot idx.
            u32 archetypeSize = static_cast<u32>(record.archetype->entityIndex.size());
            if (archetypeSize > 1 && idx < archetypeSize - 1) {
                for (auto& [eid, slot] : record.archetype->entityIndex) {
                    if (slot == record.archetype->entityIndex.size() && eid != id) {
                        slot = idx;
                        break;
                    }
                }
            }
            record.archetype->entityIndex.erase(id);
        }
    }

    m_entityRecords.erase(id);
    m_freeIDs.push_back(id);
    --m_aliveCount;
}

bool ECSWorld::isValid(EntityID id) const {
    if (id == INVALID_ENTITY) return false;
    return m_entityRecords.find(id) != m_entityRecords.end();
}

Archetype& ECSWorld::getOrCreateArchetype(const ComponentMask& mask) {
    // Use the bitset's to_string() as map key (simple, adequate for prototype).
    std::string key = mask.to_string();
    auto it = m_archetypes.find(key);
    if (it != m_archetypes.end()) return *it->second;

    auto arch  = std::make_unique<Archetype>();
    arch->mask = mask;
    Archetype* raw = arch.get();
    m_archetypes[key] = std::move(arch);
    return *raw;
}

void ECSWorld::migrateEntity(EntityID id, EntityRecord& record, Archetype& target) {
    if (record.archetype == &target) return;

    if (record.archetype) {
        // Remove from old archetype.
        auto it = record.archetype->entityIndex.find(id);
        if (it != record.archetype->entityIndex.end()) {
            u32 idx = it->second;
            // TODO: copy existing components into the target archetype columns
            // For each component in (old mask & new mask), copy data.
            for (auto& [cid, col] : record.archetype->columns) {
                col->removeEntity(idx);
            }
            record.archetype->entityIndex.erase(id);
        }
    }

    record.archetype = &target;
}

} // namespace shooter
