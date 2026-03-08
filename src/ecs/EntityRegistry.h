#pragma once

#include "ecs/Entity.h"
#include "ecs/Component.h"
#include "ecs/System.h"
#include "core/Memory.h"

#include <array>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cassert>

namespace shooter {

/// Dense component storage for a single component type.
template<typename T>
class ComponentArray final : public IComponentArray {
public:
    ComponentArray() {
        m_data.resize(MAX_ENTITIES);
        m_indexToEntity.resize(MAX_ENTITIES);
    }

    void insert(Entity e, T component) {
        assert(m_entityToIndex.find(e.id) == m_entityToIndex.end());
        u32 idx = static_cast<u32>(m_size);
        m_data[idx]              = std::move(component);
        m_indexToEntity[idx]     = e;
        m_entityToIndex[e.id]    = idx;
        ++m_size;
    }

    void removeEntity(Entity e) override {
        auto it = m_entityToIndex.find(e.id);
        if (it == m_entityToIndex.end()) return;

        u32 removedIdx = it->second;
        u32 lastIdx    = m_size - 1;

        // Swap with last element to keep array packed.
        m_data[removedIdx] = std::move(m_data[lastIdx]);
        Entity lastEntity  = m_indexToEntity[lastIdx];
        m_indexToEntity[removedIdx] = lastEntity;
        m_entityToIndex[lastEntity.id] = removedIdx;

        m_entityToIndex.erase(e.id);
        --m_size;
    }

    T* get(Entity e) {
        auto it = m_entityToIndex.find(e.id);
        if (it == m_entityToIndex.end()) return nullptr;
        return &m_data[it->second];
    }

    // Iteration: iterates only the live [0, m_size) elements.
    T*   begin() { return m_data.data(); }
    T*   end()   { return m_data.data() + m_size; }

private:
    std::vector<T>                   m_data;
    std::vector<Entity>              m_indexToEntity;
    std::unordered_map<u64, u32>     m_entityToIndex;
    u32 m_size{0};
};

// ── EntityRegistry ─────────────────────────────────────────────────────────────

/// Central registry for entities, components, and systems.
///
/// Thread safety: entity creation / destruction must happen on the main
/// thread (or while holding the registry lock).  Component reads from
/// worker threads are safe as long as no structural changes are occurring.
class EntityRegistry {
public:
    EntityRegistry();
    ~EntityRegistry() = default;

    // ── Entity management ─────────────────────────────────────────────────────
    Entity createEntity();
    void   destroyEntity(Entity e);
    bool   isAlive(Entity e) const;
    u32    entityCount() const { return m_aliveCount; }

    // ── Component management ──────────────────────────────────────────────────
    template<typename T>
    void addComponent(Entity e, T component) {
        getOrCreateArray<T>()->insert(e, std::move(component));
        m_masks[e.index()].set(T::typeId());
    }

    template<typename T>
    void removeComponent(Entity e) {
        getOrCreateArray<T>()->removeEntity(e);
        m_masks[e.index()].reset(T::typeId());
    }

    template<typename T>
    T* getComponent(Entity e) {
        return getOrCreateArray<T>()->get(e);
    }

    template<typename T>
    bool hasComponent(Entity e) const {
        if (!isAlive(e)) return false;
        return m_masks[e.index()].test(T::typeId());
    }

    const ComponentMask& getMask(Entity e) const {
        return m_masks[e.index()];
    }

    // ── System management ─────────────────────────────────────────────────────
    void addSystem(std::unique_ptr<ISystem> system);
    void updateSystems(float dt);

private:
    template<typename T>
    ComponentArray<T>* getOrCreateArray() {
        u32 tid = T::typeId();
        if (!m_componentArrays[tid]) {
            m_componentArrays[tid] = std::make_unique<ComponentArray<T>>();
        }
        return static_cast<ComponentArray<T>*>(m_componentArrays[tid].get());
    }

    // Entity handle pool.
    std::vector<u32>                   m_versions;
    std::vector<u32>                   m_freeList;
    u32                                m_aliveCount{0};
    u32                                m_nextIndex{0};

    // Component storage – one array per type.
    std::array<std::unique_ptr<IComponentArray>, MAX_COMPONENTS> m_componentArrays{};

    // Per-entity component masks.
    std::vector<ComponentMask>         m_masks;

    // Registered systems.
    std::vector<std::unique_ptr<ISystem>> m_systems;
};

} // namespace shooter
