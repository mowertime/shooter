#pragma once

#include "ecs/Entity.h"
#include <atomic>

namespace shooter {

/// Global component-type counter.  Each specialisation of ComponentTraits
/// receives a unique ID at first use.
struct ComponentTypeRegistry {
    static u32 nextId() {
        static std::atomic<u32> counter{0};
        return counter.fetch_add(1, std::memory_order_relaxed);
    }
};

/// Base class for all component pools.
struct IComponentArray {
    virtual ~IComponentArray() = default;
    virtual void removeEntity(Entity e) = 0;
};

// ── Common gameplay components ─────────────────────────────────────────────────

/// World-space transform.
struct TransformComponent {
    static u32 typeId() { static u32 id = ComponentTypeRegistry::nextId(); return id; }

    float x{0}, y{0}, z{0};         ///< Position
    float yaw{0}, pitch{0}, roll{0};  ///< Orientation (radians)
    float scaleX{1}, scaleY{1}, scaleZ{1};
};

/// Rigid body dynamics state.
struct RigidBodyComponent {
    static u32 typeId() { static u32 id = ComponentTypeRegistry::nextId(); return id; }

    float vx{0}, vy{0}, vz{0};     ///< Linear velocity (m/s)
    float ax{0}, ay{0}, az{0};     ///< Angular velocity (rad/s)
    float mass{1.0f};              ///< kg
    bool  isKinematic{false};
    bool  isSleeping{false};
};

/// Health and damage model.
struct HealthComponent {
    static u32 typeId() { static u32 id = ComponentTypeRegistry::nextId(); return id; }

    float current{100.0f};
    float maximum{100.0f};
    float armour{0.0f};             ///< Armour rating (absorbs damage)
    bool  isAlive() const { return current > 0.0f; }
};

/// AI brain link – references the AI controller index.
struct AIComponent {
    static u32 typeId() { static u32 id = ComponentTypeRegistry::nextId(); return id; }

    u32  controllerId{UINT32_MAX};
    u8   factionId{0};
    bool isActive{true};
};

/// Render mesh reference.
struct MeshComponent {
    static u32 typeId() { static u32 id = ComponentTypeRegistry::nextId(); return id; }

    u32  meshAssetId{UINT32_MAX};
    u32  materialId{UINT32_MAX};
    bool castsShadow{true};
    bool isVisible{true};
};

/// Weapon held by an entity.
struct WeaponComponent {
    static u32 typeId() { static u32 id = ComponentTypeRegistry::nextId(); return id; }

    u32   weaponTypeId{UINT32_MAX};
    i32   ammoInMag{30};
    i32   ammoReserve{120};
    float reloadTimer{0.0f};
    float fireRateTimer{0.0f};
};

/// Vehicle occupancy.
struct VehicleComponent {
    static u32 typeId() { static u32 id = ComponentTypeRegistry::nextId(); return id; }

    u32   vehicleTypeId{UINT32_MAX};
    float engineRpm{0.0f};
    float speed{0.0f};         ///< m/s
    float fuel{100.0f};        ///< percentage
    u8    occupantCount{0};
    bool  isDestroyed{false};
};

} // namespace shooter
