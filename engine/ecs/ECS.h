#pragma once

// ---------------------------------------------------------------------------
// ECS.h — Archetype-based Entity Component System.
//
// Design:
//   • Entities are just 64-bit IDs.
//   • Components are plain-old-data structs stored contiguously per Archetype.
//   • An Archetype is a unique combination of component types; its storage is
//     a Structure-of-Arrays so each component array is cache-friendly.
//   • Systems iterate archetypes that match their required component mask.
//
// This layout is inspired by Unity DOTS / Flecs and is well-suited for
// hundreds of AI units that share the same component sets.
// ---------------------------------------------------------------------------

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <string>
#include <bitset>
#include <array>

#include "engine/platform/Platform.h"

namespace shooter {

// ---------------------------------------------------------------------------
// Type IDs
// ---------------------------------------------------------------------------
using EntityID    = u64;
using ComponentID = u32;

static constexpr EntityID    INVALID_ENTITY    = 0;
static constexpr ComponentID MAX_COMPONENTS    = 64;
static constexpr u32         MAX_ENTITIES      = 1'000'000;

using ComponentMask = std::bitset<MAX_COMPONENTS>;

// ---------------------------------------------------------------------------
// ComponentRegistry — maps C++ type to a stable ComponentID at runtime.
// ---------------------------------------------------------------------------
class ComponentRegistry {
public:
    template<typename T>
    static ComponentID id() {
        static ComponentID cid = nextID();
        return cid;
    }

private:
    static ComponentID nextID() {
        static ComponentID counter = 0;
        SE_ASSERT(counter < MAX_COMPONENTS, "Too many component types registered");
        return counter++;
    }
};

// ---------------------------------------------------------------------------
// Built-in components
// ---------------------------------------------------------------------------

/// 3D transform: position, rotation (quaternion), scale.
struct TransformComponent {
    f32 x{0}, y{0}, z{0};           ///< world position
    f32 qx{0}, qy{0}, qz{0}, qw{1}; ///< rotation quaternion
    f32 sx{1}, sy{1}, sz{1};         ///< scale
};

/// Linear and angular velocity.
struct VelocityComponent {
    f32 vx{0}, vy{0}, vz{0};   ///< linear velocity (m/s)
    f32 ax{0}, ay{0}, az{0};   ///< angular velocity (rad/s)
};

/// Links an entity to a renderable mesh/material pair.
struct RenderableComponent {
    u32  meshID{0};
    u32  materialID{0};
    bool visible{true};
    u32  lodLevel{0};
};

/// Hit points and damage model.
struct HealthComponent {
    f32 current{100.f};
    f32 maximum{100.f};
    f32 armour{0.f};    ///< damage reduction factor [0,1]
    bool isDead() const { return current <= 0.f; }
};

/// Marks an entity as AI-controlled and stores a handle to its behaviour tree.
struct AIComponent {
    u32  behaviourTreeID{0};
    u32  squadID{0};
    f32  reactionTime{0.3f};   ///< seconds before noticing a threat
    bool isActive{true};
};

// ---------------------------------------------------------------------------
// IComponentArray — type-erased interface for component storage vectors.
// ---------------------------------------------------------------------------
class IComponentArray {
public:
    virtual ~IComponentArray() = default;
    virtual void removeEntity(u32 denseIndex) = 0;
    virtual u32  size()                  const = 0;
};

// ComponentArray<T> stores a contiguous vector of T plus a parallel entity list.
template<typename T>
class ComponentArray : public IComponentArray {
public:
    std::vector<T>        data;
    std::vector<EntityID> owners;   ///< data[i] belongs to owners[i]

    u32 push(EntityID entity, T value) {
        u32 idx = static_cast<u32>(data.size());
        data.push_back(std::move(value));
        owners.push_back(entity);
        return idx;
    }

    /// Swap-erase to maintain contiguity.
    void removeEntity(u32 denseIndex) override {
        data[denseIndex]   = std::move(data.back());
        owners[denseIndex] = owners.back();
        data.pop_back();
        owners.pop_back();
    }

    u32 size() const override { return static_cast<u32>(data.size()); }
};

// ---------------------------------------------------------------------------
// Archetype — stores all entities sharing a specific ComponentMask.
// Each component type has its own ComponentArray stored by ComponentID.
// ---------------------------------------------------------------------------
struct Archetype {
    ComponentMask                                     mask;
    std::unordered_map<ComponentID,
        std::unique_ptr<IComponentArray>>             columns;
    /// Dense index: entity → row index within component arrays.
    std::unordered_map<EntityID, u32>                 entityIndex;

    template<typename T>
    ComponentArray<T>* getOrCreateColumn() {
        ComponentID cid = ComponentRegistry::id<T>();
        auto it = columns.find(cid);
        if (it == columns.end()) {
            auto arr = std::make_unique<ComponentArray<T>>();
            auto* raw = arr.get();
            columns[cid] = std::move(arr);
            return raw;
        }
        return static_cast<ComponentArray<T>*>(it->second.get());
    }

    template<typename T>
    ComponentArray<T>* getColumn() {
        ComponentID cid = ComponentRegistry::id<T>();
        auto it = columns.find(cid);
        return it != columns.end()
            ? static_cast<ComponentArray<T>*>(it->second.get())
            : nullptr;
    }
};

// ---------------------------------------------------------------------------
// ECSWorld — the central ECS registry.
// ---------------------------------------------------------------------------
class ECSWorld {
public:
    ECSWorld();

    // ---- Entity lifecycle --------------------------------------------------
    EntityID createEntity();
    void     destroyEntity(EntityID id);

    // ---- Component management ----------------------------------------------
    template<typename T>
    void addComponent(EntityID id, T value = T{}) {
        SE_ASSERT(isValid(id), "Invalid entity");
        ComponentID cid = ComponentRegistry::id<T>();

        auto& record = m_entityRecords[id];
        ComponentMask newMask = record.mask;
        newMask.set(cid);

        // Move entity to the matching archetype.
        Archetype& arch = getOrCreateArchetype(newMask);
        migrateEntity(id, record, arch);
        arch.getOrCreateColumn<T>()->push(id, std::move(value));
        arch.entityIndex[id] = arch.getOrCreateColumn<T>()->size() - 1;
        record.mask = newMask;
    }

    template<typename T>
    void removeComponent(EntityID id) {
        SE_ASSERT(isValid(id), "Invalid entity");
        ComponentID cid = ComponentRegistry::id<T>();
        auto& record = m_entityRecords[id];
        if (!record.mask.test(cid)) return;

        ComponentMask newMask = record.mask;
        newMask.reset(cid);
        Archetype& arch = getOrCreateArchetype(newMask);
        migrateEntityWithout<T>(id, record, arch);
        record.mask = newMask;
    }

    template<typename T>
    T* getComponent(EntityID id) {
        if (!isValid(id)) return nullptr;
        auto& record = m_entityRecords[id];
        ComponentID cid = ComponentRegistry::id<T>();
        if (!record.mask.test(cid)) return nullptr;
        Archetype* arch = record.archetype;
        if (!arch) return nullptr;
        auto* col = arch->getColumn<T>();
        if (!col) return nullptr;
        u32 idx = arch->entityIndex[id];
        return &col->data[idx];
    }

    template<typename T>
    bool hasComponent(EntityID id) const {
        if (!isValid(id)) return false;
        ComponentID cid = ComponentRegistry::id<T>();
        return m_entityRecords.at(id).mask.test(cid);
    }

    // ---- Iteration ---------------------------------------------------------
    /// Call func(EntityID, T&) for every entity that has component T.
    template<typename T>
    void each(std::function<void(EntityID, T&)> func) {
        ComponentID cid = ComponentRegistry::id<T>();
        for (auto& [key, arch] : m_archetypes) {
            if (!arch->mask.test(cid)) continue;
            auto* col = arch->getColumn<T>();
            if (!col) continue;
            for (u32 i = 0; i < col->size(); ++i) {
                func(col->owners[i], col->data[i]);
            }
        }
    }

    /// Call func(EntityID, T1&, T2&) for every entity that has both T1 and T2.
    template<typename T1, typename T2>
    void each(std::function<void(EntityID, T1&, T2&)> func) {
        ComponentID c1 = ComponentRegistry::id<T1>();
        ComponentID c2 = ComponentRegistry::id<T2>();
        for (auto& [key, arch] : m_archetypes) {
            if (!arch->mask.test(c1) || !arch->mask.test(c2)) continue;
            auto* col1 = arch->getColumn<T1>();
            auto* col2 = arch->getColumn<T2>();
            if (!col1 || !col2) continue;
            for (u32 i = 0; i < col1->size(); ++i) {
                func(col1->owners[i], col1->data[i], col2->data[i]);
            }
        }
    }

    [[nodiscard]] bool   isValid(EntityID id) const;
    [[nodiscard]] u32    entityCount() const { return m_aliveCount; }

private:
    struct EntityRecord {
        ComponentMask mask;
        Archetype*    archetype{nullptr};
    };

    Archetype& getOrCreateArchetype(const ComponentMask& mask);

    void migrateEntity(EntityID id, EntityRecord& record, Archetype& target);

    template<typename T>
    void migrateEntityWithout(EntityID id, EntityRecord& record, Archetype& target) {
        // Remove from old archetype, add to new without component T
        (void)id; (void)record; (void)target;
        // TODO: implement full column-by-column copy for migration
    }

    std::unordered_map<EntityID, EntityRecord>     m_entityRecords;
    // Archetype map keyed by mask's string representation (simple prototype key).
    std::unordered_map<std::string, std::unique_ptr<Archetype>> m_archetypes;

    EntityID m_nextEntityID{1};
    u32      m_aliveCount{0};

    // Free list for recycling IDs.
    std::vector<EntityID> m_freeIDs;
};

// ---------------------------------------------------------------------------
// System — base class for all simulation systems.
// ---------------------------------------------------------------------------
class System {
public:
    virtual ~System() = default;
    virtual void update(ECSWorld& world, f32 dt) = 0;
    [[nodiscard]] virtual const char* name() const = 0;
};

// ---------------------------------------------------------------------------
// SystemRegistry — owns and ticks all registered systems in order.
// ---------------------------------------------------------------------------
class SystemRegistry {
public:
    template<typename T, typename... Args>
    T* registerSystem(Args&&... args) {
        auto sys = std::make_unique<T>(std::forward<Args>(args)...);
        T* raw = sys.get();
        m_systems.push_back(std::move(sys));
        return raw;
    }

    void update(ECSWorld& world, f32 dt) {
        for (auto& sys : m_systems) {
            sys->update(world, dt);
        }
    }

private:
    std::vector<std::unique_ptr<System>> m_systems;
};

} // namespace shooter
