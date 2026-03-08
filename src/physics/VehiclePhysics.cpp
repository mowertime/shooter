#include "physics/VehiclePhysics.h"
#include "core/Logger.h"
#include <cmath>

namespace shooter {

// ── VehiclePhysics ────────────────────────────────────────────────────────────

u32 VehiclePhysics::createVehicle(PhysicsWorld& world,
                                   const VehicleParams& params,
                                   float x, float y, float z) {
    u32 bodyId = world.addRigidBody(x, y, z, params.mass);
    VehicleState vs{};
    vs.bodyId = bodyId;
    vs.params = params;
    u32 id = static_cast<u32>(m_vehicles.size());
    m_vehicles.push_back(vs);
    LOG_DEBUG("VehiclePhysics", "Created vehicle id=%u bodyId=%u", id, bodyId);
    return id;
}

void VehiclePhysics::update(PhysicsWorld& world, float dt) {
    for (auto& v : m_vehicles) {
        float px, py, pz;
        if (!world.getPosition(v.bodyId, px, py, pz)) continue;

        // Simplified engine torque → wheel force → acceleration model.
        float driveForce = v.params.maxEngineTorque * v.throttle
                         - v.params.brakeTorque * v.brake;
        // Aerodynamic drag.
        float drag = v.params.dragCoeff * v.speed * v.speed;
        float netForce = driveForce - drag;
        float accel    = netForce / v.params.mass;
        v.speed  = std::max(0.0f, v.speed + accel * dt);
        v.engineRpm = 800.0f + v.speed * 100.0f;

        // Apply forward velocity impulse to physics body.
        world.applyImpulse(v.bodyId, 0.0f, 0.0f, v.speed * dt);
    }
}

void VehiclePhysics::setControls(u32 id, float throttle,
                                  float steer, float brake) {
    if (id >= m_vehicles.size()) return;
    m_vehicles[id].throttle = throttle;
    m_vehicles[id].steer    = steer;
    m_vehicles[id].brake    = brake;
}

void VehiclePhysics::destroyVehicle(PhysicsWorld& world, u32 id) {
    if (id >= m_vehicles.size()) return;
    world.removeBody(m_vehicles[id].bodyId);
    m_vehicles.erase(m_vehicles.begin() + static_cast<ptrdiff_t>(id));
}

// ── AircraftPhysics ───────────────────────────────────────────────────────────

u32 AircraftPhysics::createAircraft(PhysicsWorld& world,
                                     const AircraftParams& params,
                                     float x, float y, float z) {
    u32 bodyId = world.addRigidBody(x, y, z, params.mass);
    AircraftState as{};
    as.bodyId = bodyId;
    as.params = params;
    u32 id = static_cast<u32>(m_aircraft.size());
    m_aircraft.push_back(as);
    return id;
}

void AircraftPhysics::update(PhysicsWorld& world, float dt) {
    for (auto& a : m_aircraft) {
        float px, py, pz;
        if (!world.getPosition(a.bodyId, px, py, pz)) continue;

        // Simplified lift and drag model.
        float thrust = a.params.maxThrust * a.throttle;
        float liftFactor = a.params.liftCoeffBase * a.params.wingArea;
        float liftForce  = liftFactor * a.airspeed * a.airspeed * 0.5f * 1.225f;
        float dragForce  = a.params.dragCoeffBase * a.params.wingArea
                         * a.airspeed * a.airspeed * 0.5f * 1.225f;

        float netForceZ = thrust - dragForce;
        float netForceY = liftForce - a.params.mass * 9.81f;

        float accelZ = netForceZ / a.params.mass;
        float accelY = netForceY / a.params.mass;

        a.airspeed = std::max(0.0f, a.airspeed + accelZ * dt);
        world.applyImpulse(a.bodyId,
                           0.0f,
                           accelY * dt * a.params.mass,
                           accelZ * dt * a.params.mass);
    }
}

void AircraftPhysics::setControls(u32 id, float pitch, float roll,
                                   float yaw, float throttle) {
    if (id >= m_aircraft.size()) return;
    m_aircraft[id].pitch    = pitch;
    m_aircraft[id].roll     = roll;
    m_aircraft[id].yaw      = yaw;
    m_aircraft[id].throttle = throttle;
}

void AircraftPhysics::destroyAircraft(PhysicsWorld& world, u32 id) {
    if (id >= m_aircraft.size()) return;
    world.removeBody(m_aircraft[id].bodyId);
    m_aircraft.erase(m_aircraft.begin() + static_cast<ptrdiff_t>(id));
}

} // namespace shooter
