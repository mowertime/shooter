// ---------------------------------------------------------------------------
// Physics.cpp — PhysicsWorld implementation.
// ---------------------------------------------------------------------------

#include "engine/physics/Physics.h"
#include <iostream>
#include <algorithm>

namespace shooter {

bool PhysicsWorld::initialize() {
    std::cout << "[Physics] Initialized (prototype solver)\n";
    // TODO: initialise PhysX / Bullet SDK if used
    return true;
}

void PhysicsWorld::shutdown() {
    m_rigidBodies.clear();
    m_vehicles.clear();
    m_aircraft.clear();
    std::cout << "[Physics] Shutdown\n";
}

// ---------------------------------------------------------------------------
// Body management
// ---------------------------------------------------------------------------

void PhysicsWorld::addRigidBody(const RigidBodyDesc& desc) {
    RigidBody rb;
    rb.entity    = desc.entity;
    rb.position  = desc.initialPosition;
    rb.mass      = desc.mass;
    rb.isStatic  = (desc.mass == 0.f);
    rb.isKinematic = desc.isKinematic;
    rb.inverseMass = rb.isStatic ? 0.f : 1.f / desc.mass;
    rb.shape     = desc.shape;
    m_rigidBodies[desc.entity] = rb;
}

void PhysicsWorld::addSoftBody(EntityID entity) {
    // TODO: create FEM/PBD soft body for cloth, vehicle tyres, etc.
    std::cout << "[Physics] addSoftBody entity=" << entity << " (stub)\n";
}

void PhysicsWorld::addVehicle(EntityID entity, const VehicleConfig& config) {
    // Also create an underlying rigid body for the chassis.
    RigidBodyDesc chassis;
    chassis.entity = entity;
    chassis.mass   = config.mass;
    chassis.shape  = { PhysicsShape::Box, 0.f, {2.5f, 1.f, 5.f} };
    addRigidBody(chassis);
    m_vehicles[entity] = config;
}

void PhysicsWorld::addAircraft(EntityID entity, const AircraftConfig& config) {
    RigidBodyDesc airframe;
    airframe.entity = entity;
    airframe.mass   = config.mass;
    airframe.shape  = { PhysicsShape::ConvexMesh };
    addRigidBody(airframe);
    m_aircraft[entity] = config;
}

void PhysicsWorld::removeBody(EntityID entity) {
    m_rigidBodies.erase(entity);
    m_vehicles.erase(entity);
    m_aircraft.erase(entity);
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

Vec3 PhysicsWorld::getPosition(EntityID entity) const {
    auto it = m_rigidBodies.find(entity);
    return it != m_rigidBodies.end() ? it->second.position : Vec3{};
}

Vec3 PhysicsWorld::getVelocity(EntityID entity) const {
    auto it = m_rigidBodies.find(entity);
    return it != m_rigidBodies.end() ? it->second.velocity : Vec3{};
}

void PhysicsWorld::setPosition(EntityID entity, Vec3 pos) {
    auto it = m_rigidBodies.find(entity);
    if (it != m_rigidBodies.end()) it->second.position = pos;
}

void PhysicsWorld::applyImpulse(EntityID entity, Vec3 impulse) {
    auto it = m_rigidBodies.find(entity);
    if (it == m_rigidBodies.end() || it->second.isStatic) return;
    RigidBody& rb = it->second;
    // Δv = impulse / mass
    rb.velocity += impulse * rb.inverseMass;
}

// ---------------------------------------------------------------------------
// Ballistics solver
//
// Uses a 4th-order Runge-Kutta integrator with the drag equation:
//   F_drag = 0.5 * ρ * v² * Cd * A  (opposing velocity)
//   F_gravity = m * g  (downward)
//   F_wind = modelled as a constant ambient velocity added to relative air speed
// ---------------------------------------------------------------------------
std::vector<TrajectoryPoint> PhysicsWorld::simulateProjectile(
    const ProjectileDesc& desc)
{
    std::vector<TrajectoryPoint> trajectory;

    const f32 dt      = 0.01f;   // 10 ms integration step
    const i32 steps   = static_cast<i32>(desc.simulationTime / dt);
    const f32 invMass = 1.f / desc.mass;

    Vec3 pos = desc.origin;
    Vec3 vel = desc.direction.normalised() * desc.muzzleVelocity;

    trajectory.reserve(static_cast<size_t>(steps));

    auto acceleration = [&](Vec3 v) -> Vec3 {
        // Relative velocity of bullet through the air
        Vec3 relVel = v - desc.wind;
        f32  speed  = relVel.length();
        // Drag force magnitude: F = 0.5 * rho * v² * Cd * A
        f32 dragMag = 0.5f * AIR_DENSITY * speed * speed
                    * desc.dragCoefficient * desc.crossSection;
        Vec3 dragAcc = relVel.normalised() * (-dragMag * invMass);
        // Gravity
        Vec3 gravAcc = {0.f, GRAVITY, 0.f};
        return dragAcc + gravAcc;
    };

    // RK4 integration
    for (i32 i = 0; i < steps; ++i) {
        TrajectoryPoint pt;
        pt.position = pos;
        pt.velocity = vel;
        pt.time     = i * dt;
        trajectory.push_back(pt);

        // Ground check (y = 0 = sea level; terrain height would replace this)
        if (pos.y < 0.f) break;

        // k1
        Vec3 k1v = acceleration(vel);
        Vec3 k1p = vel;
        // k2
        Vec3 k2v = acceleration(vel + k1v * (dt * 0.5f));
        Vec3 k2p = vel + k1v * (dt * 0.5f);
        // k3
        Vec3 k3v = acceleration(vel + k2v * (dt * 0.5f));
        Vec3 k3p = vel + k2v * (dt * 0.5f);
        // k4
        Vec3 k4v = acceleration(vel + k3v * dt);
        Vec3 k4p = vel + k3v * dt;

        vel = vel + (k1v + k2v * 2.f + k3v * 2.f + k4v) * (dt / 6.f);
        pos = pos + (k1p + k2p * 2.f + k3p * 2.f + k4p) * (dt / 6.f);
    }

    return trajectory;
}

// ---------------------------------------------------------------------------
// Simulation step
// ---------------------------------------------------------------------------

void PhysicsWorld::step(f32 dt) {
    integrateRigidBodies(dt);
    broadPhase();
    narrowPhase();
    solveConstraints(dt);

    // Integrate aircraft with simplified aerodynamics.
    for (auto& [eid, cfg] : m_aircraft) {
        auto it = m_rigidBodies.find(eid);
        if (it == m_rigidBodies.end()) continue;
        RigidBody& rb = it->second;
        f32 speed = rb.velocity.length();
        // Simplified lift: L = 0.5 * rho * v² * Cl * A
        f32 lift = 0.5f * AIR_DENSITY * speed * speed
                 * cfg.liftCoefficient * cfg.wingArea;
        f32 drag = 0.5f * AIR_DENSITY * speed * speed
                 * cfg.dragCoefficient * cfg.wingArea;
        // Apply forces (upward lift, rearward drag, gravity)
        Vec3 gravForce = {0.f, GRAVITY * cfg.mass, 0.f};
        Vec3 liftForce = {0.f, lift, 0.f};
        Vec3 dragForce = rb.velocity.normalised() * (-drag);
        Vec3 totalAcc  = (gravForce + liftForce + dragForce) * rb.inverseMass;
        rb.velocity += totalAcc * dt;
        rb.position += rb.velocity * dt;
    }
}

void PhysicsWorld::integrateRigidBodies(f32 dt) {
    for (auto& [eid, rb] : m_rigidBodies) {
        if (rb.isStatic || rb.isKinematic) continue;
        // Apply gravity
        rb.velocity.y += GRAVITY * dt;
        // Semi-implicit Euler
        rb.position += rb.velocity * dt;
        // Clamp to ground plane (placeholder terrain floor at y = 0)
        if (rb.position.y < 0.f) {
            rb.position.y = 0.f;
            rb.velocity.y = 0.f;
            // Apply simple friction
            rb.velocity.x *= 0.98f;
            rb.velocity.z *= 0.98f;
        }
    }
}

void PhysicsWorld::broadPhase() {
    // TODO: sweep-and-prune along X axis; insert collision pairs into narrow phase queue.
}

void PhysicsWorld::narrowPhase() {
    // TODO: GJK + EPA for convex shapes; SAT for box-box; sphere overlap.
}

void PhysicsWorld::solveConstraints(f32 dt) {
    (void)dt;
    // TODO: PGS (projected Gauss-Seidel) constraint solver: contact, joints, vehicles.
}

} // namespace shooter
