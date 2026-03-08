#pragma once

#include "physics/RigidBody.h"
#include "physics/PhysicsWorld.h"

namespace shooter {

/// Vehicle dynamics integrator.
///
/// Simulates a 4-wheel vehicle using a simplified Pacejka tyre model
/// and a spring-damper suspension per wheel.
class VehiclePhysics {
public:
    /// Create a vehicle and register its bodies with \p world.
    u32 createVehicle(PhysicsWorld& world, const VehicleParams& params,
                      float x, float y, float z);

    /// Update all vehicles.  \p dt is the physics timestep (seconds).
    void update(PhysicsWorld& world, float dt);

    /// Apply throttle [-1..1] and steering [-1..1] inputs to vehicle \p id.
    void setControls(u32 vehicleId, float throttle, float steer, float brake);

    /// Destroy a vehicle and remove its bodies from the world.
    void destroyVehicle(PhysicsWorld& world, u32 vehicleId);

    /// Return the physics body id for a vehicle.
    u32  bodyId(u32 vehicleId) const {
        if (vehicleId >= m_vehicles.size()) return UINT32_MAX;
        return m_vehicles[vehicleId].bodyId;
    }

private:
    struct VehicleState {
        u32          bodyId;
        VehicleParams params;
        float        throttle{0};
        float        steer{0};
        float        brake{0};
        float        engineRpm{800};
        float        speed{0};
        WheelContactInfo wheels[4];
    };
    std::vector<VehicleState> m_vehicles;
};

/// Fixed-wing and rotary aircraft physics.
class AircraftPhysics {
public:
    u32  createAircraft(PhysicsWorld& world, const AircraftParams& params,
                        float x, float y, float z);
    void update(PhysicsWorld& world, float dt);
    void setControls(u32 aircraftId, float pitch, float roll, float yaw,
                     float throttle);
    void destroyAircraft(PhysicsWorld& world, u32 aircraftId);

    /// Return the physics body id for an aircraft.
    u32  bodyId(u32 aircraftId) const {
        if (aircraftId >= m_aircraft.size()) return UINT32_MAX;
        return m_aircraft[aircraftId].bodyId;
    }

private:
    struct AircraftState {
        u32           bodyId;
        AircraftParams params;
        float         pitch{0}, roll{0}, yaw{0}, throttle{0};
        float         airspeed{0};
    };
    std::vector<AircraftState> m_aircraft;
};

} // namespace shooter
