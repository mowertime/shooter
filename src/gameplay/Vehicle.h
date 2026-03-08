#pragma once

#include "ecs/EntityRegistry.h"
#include "physics/VehiclePhysics.h"
#include "physics/AircraftPhysics.h"

namespace shooter {

/// Vehicle categories.
enum class VehicleCategory : u8 {
    Tank,
    APC,
    Truck,
    Helicopter,
    FixedWingJet,
    Boat,
    Motorcycle,
};

/// Descriptor for a vehicle type.
struct VehicleDesc {
    std::string      name;
    VehicleCategory  category{VehicleCategory::APC};
    float            maxHealth{1000.0f};
    u8               maxOccupants{4};
    u32              meshAssetId{UINT32_MAX};
};

/// Vehicle instance.  Ties together an ECS entity, physics body, and driver AI.
class VehicleInstance {
public:
    VehicleInstance(EntityRegistry& registry, PhysicsWorld& physicsWorld,
                    VehiclePhysics& vehiclePhysics, AircraftPhysics& aircraftPhysics,
                    const VehicleDesc& desc, float x, float y, float z);
    ~VehicleInstance();

    void update(float dt);

    /// Apply driver input.
    void setInput(float throttle, float steer, float brake);

    Entity  entity()   const { return m_entity;    }
    bool    isAlive()  const;
    float   health()   const;

    void applyDamage(float dmg);

private:
    EntityRegistry& m_registry;
    PhysicsWorld&   m_physics;
    VehiclePhysics& m_vehiclePhysics;
    AircraftPhysics& m_aircraftPhysics;

    Entity      m_entity;
    VehicleDesc m_desc;
    u32         m_vehicleId{UINT32_MAX};
    bool        m_isAircraft{false};
};

} // namespace shooter
