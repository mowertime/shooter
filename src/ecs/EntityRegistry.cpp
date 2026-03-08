#include "ecs/EntityRegistry.h"
#include "core/Logger.h"

namespace shooter {

// ── EntityRegistry ────────────────────────────────────────────────────────────

EntityRegistry::EntityRegistry() {
    m_versions.resize(MAX_ENTITIES, 0);
    m_masks.resize(MAX_ENTITIES);
    m_freeList.reserve(65536);
}

Entity EntityRegistry::createEntity() {
    u32 index;
    if (!m_freeList.empty()) {
        index = m_freeList.back();
        m_freeList.pop_back();
    } else {
        assert(m_nextIndex < MAX_ENTITIES && "Entity limit reached");
        index = m_nextIndex++;
        m_versions[index] = 0;
    }
    ++m_aliveCount;
    Entity e;
    e.id = (static_cast<u64>(m_versions[index]) << 32) | index;
    return e;
}

void EntityRegistry::destroyEntity(Entity e) {
    if (!isAlive(e)) return;
    u32 idx = e.index();
    ++m_versions[idx];            // invalidate existing handles
    m_masks[idx].reset();
    // Remove from all component arrays.
    for (auto& arr : m_componentArrays) {
        if (arr) arr->removeEntity(e);
    }
    m_freeList.push_back(idx);
    --m_aliveCount;
}

bool EntityRegistry::isAlive(Entity e) const {
    if (!e.isValid()) return false;
    u32 idx = e.index();
    return idx < m_nextIndex && m_versions[idx] == e.version();
}

void EntityRegistry::addSystem(std::unique_ptr<ISystem> system) {
    m_systems.push_back(std::move(system));
}

void EntityRegistry::updateSystems(float dt) {
    for (auto& sys : m_systems) {
        sys->update(*this, dt);
    }
}

// ── Built-in system implementations ──────────────────────────────────────────

ComponentMask PhysicsIntegrationSystem::signature() const {
    ComponentMask m;
    m.set(TransformComponent::typeId());
    m.set(RigidBodyComponent::typeId());
    return m;
}

void PhysicsIntegrationSystem::update(EntityRegistry& registry, float dt) {
    // In a production implementation this would iterate a sorted,
    // SIMD-friendly SoA layout.  Here we demonstrate the pattern.
    (void)registry; (void)dt;
}

ComponentMask AIUpdateSystem::signature() const {
    ComponentMask m;
    m.set(TransformComponent::typeId());
    m.set(AIComponent::typeId());
    return m;
}

void AIUpdateSystem::update(EntityRegistry& registry, float dt) {
    (void)registry; (void)dt;
}

ComponentMask WeaponSystem::signature() const {
    ComponentMask m;
    m.set(WeaponComponent::typeId());
    return m;
}

void WeaponSystem::update(EntityRegistry& registry, float dt) {
    (void)registry; (void)dt;
}

} // namespace shooter
