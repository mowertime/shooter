// ---------------------------------------------------------------------------
// Vehicle.cpp — Vehicle system implementation.
// ---------------------------------------------------------------------------

#include "gameplay/Vehicle.h"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace shooter {

// ===========================================================================
// TankController
// ===========================================================================

void TankController::update(VehicleInstance& tank, f32 throttle, f32 steer, f32 dt) {
    if (tank.state == VehicleState::Destroyed || tank.fuel <= 0.f) return;

    const VehicleConfig& cfg = tank.desc.physicsConfig;

    // Differential steering: one track faster/slower.
    f32 leftTrack  = throttle - steer * 0.5f;
    f32 rightTrack = throttle + steer * 0.5f;
    f32 netThrottle = (leftTrack + rightTrack) * 0.5f;
    f32 netSteer    = (rightTrack - leftTrack);

    // Update heading.
    tank.heading += netSteer * 0.8f * dt;

    // Forward velocity from throttle + engine torque.
    f32 forceN   = netThrottle * cfg.engineTorque / cfg.wheelRadius;
    f32 accel    = forceN / cfg.mass;
    f32 sinH     = std::sin(tank.heading);
    f32 cosH     = std::cos(tank.heading);

    tank.velocity.x += cosH * accel * dt;
    tank.velocity.z += sinH * accel * dt;

    // Simple drag.
    tank.velocity.x *= 0.98f;
    tank.velocity.z *= 0.98f;

    // Clamp to max speed.
    f32 speed = std::sqrt(tank.velocity.x*tank.velocity.x +
                          tank.velocity.z*tank.velocity.z);
    if (speed > cfg.maxSpeed) {
        f32 scale = cfg.maxSpeed / speed;
        tank.velocity.x *= scale;
        tank.velocity.z *= scale;
    }

    tank.position.x += tank.velocity.x * dt;
    tank.position.z += tank.velocity.z * dt;

    // Fuel consumption.
    tank.fuel -= speed * tank.desc.fuelConsumptionRate * dt / 1000.f;
    tank.fuel  = std::max(0.f, tank.fuel);

    if (speed > 0.01f) tank.state = VehicleState::Moving;
    else               tank.state = VehicleState::Idle;

    m_gunCooldown = std::max(0.f, m_gunCooldown - dt);
}

void TankController::firePrimaryWeapon(VehicleInstance& tank,
                                        std::vector<ProjectileDesc>& outProjectiles) {
    if (m_gunCooldown > 0.f) return;

    // APFSDS round: ~1700 m/s muzzle velocity, 4.5 kg penetrator.
    ProjectileDesc proj;
    proj.origin         = tank.position;
    proj.origin.y       += 2.5f; // muzzle height
    proj.direction      = { std::cos(tank.heading), 0.f, std::sin(tank.heading) };
    proj.muzzleVelocity = 1700.f;
    proj.mass           = 4.5f;
    proj.dragCoefficient= 0.03f;  // fin-stabilised = very low drag
    proj.crossSection   = 3e-4f;
    proj.simulationTime = 5.f;
    outProjectiles.push_back(proj);

    m_gunCooldown = 8.f; // 120 mm gun reloads in ~8 s

    std::cout << "[Tank] " << tank.desc.name << " fired main gun!\n";
}

// ===========================================================================
// HelicopterController
// ===========================================================================

void HelicopterController::update(VehicleInstance& heli,
                                   f32 collective, f32 cyclicX, f32 cyclicZ,
                                   f32 pedal, f32 dt) {
    if (heli.state == VehicleState::Destroyed) return;

    const f32 mass  = heli.desc.physicsConfig.mass;
    const f32 G     = 9.81f;

    // Target rotor RPM from collective.
    f32 targetRPM = collective * 600.f; // max 600 rpm
    m_rotorRPM   += (targetRPM - m_rotorRPM) * 2.f * dt;

    // Lift force: L = (rpm/600)² * mass * G * 1.3 (overpower factor).
    f32 liftRatio = (m_rotorRPM / 600.f);
    f32 liftForce = liftRatio * liftRatio * mass * G * 1.3f;
    f32 gravForce = mass * G;
    f32 netVertAcc = (liftForce - gravForce) / mass;

    heli.velocity.y += netVertAcc * dt;

    // Cyclic: tilt to get horizontal thrust.
    heli.velocity.x += cyclicX * 5.f * dt;
    heli.velocity.z += cyclicZ * 5.f * dt;

    // Yaw via pedal (tail rotor).
    heli.heading += pedal * 1.2f * dt;

    // Drag.
    heli.velocity.x *= 0.97f;
    heli.velocity.z *= 0.97f;
    heli.velocity.y *= 0.99f;

    heli.position.x += heli.velocity.x * dt;
    heli.position.y += heli.velocity.y * dt;
    heli.position.z += heli.velocity.z * dt;

    if (heli.position.y < 0.f) {
        heli.position.y = 0.f;
        heli.velocity.y = 0.f;
    }
}

// ===========================================================================
// JetController
// ===========================================================================

void JetController::update(VehicleInstance& jet,
                             f32 throttle, f32 pitch, f32 roll, f32 yaw,
                             f32 dt) {
    if (jet.state == VehicleState::Destroyed) return;

    const f32 mass    = jet.desc.physicsConfig.mass;
    const f32 maxThrust = jet.desc.physicsConfig.engineTorque; // misusing as thrust N

    // Heading integration.
    jet.heading += yaw * 1.5f * dt;
    f32 sinH = std::sin(jet.heading);
    f32 cosH = std::cos(jet.heading);

    // Thrust along heading.
    f32 thrust = throttle * maxThrust;
    f32 accel  = thrust / mass;

    jet.velocity.x += cosH * accel * dt;
    jet.velocity.z += sinH * accel * dt;
    jet.velocity.y += pitch * 30.f * dt; // pitch authority

    // Drag.
    f32 speed = std::sqrt(jet.velocity.x*jet.velocity.x +
                          jet.velocity.z*jet.velocity.z +
                          jet.velocity.y*jet.velocity.y);
    const f32 Cd = 0.025f; // low-drag fighter airframe
    f32 drag  = 0.5f * 1.225f * speed * speed * Cd * 20.f; // wing area 20 m²
    if (speed > 1.f) {
        f32 dragAcc = drag / mass;
        jet.velocity.x -= (jet.velocity.x / speed) * dragAcc * dt;
        jet.velocity.z -= (jet.velocity.z / speed) * dragAcc * dt;
        jet.velocity.y -= (jet.velocity.y / speed) * dragAcc * dt;
    }

    jet.velocity.y -= 9.81f * dt; // gravity

    jet.position.x += jet.velocity.x * dt;
    jet.position.y += jet.velocity.y * dt;
    jet.position.z += jet.velocity.z * dt;

    if (jet.position.y < 0.f) { jet.position.y = 0.f; jet.velocity.y = 0.f; }
    (void)roll;
}

void JetController::deployWeapon(VehicleInstance& jet,
                                  std::vector<ProjectileDesc>& outProjectiles) {
    // AGM-65 Maverick stub: slow but heavy missile.
    ProjectileDesc proj;
    proj.origin         = jet.position;
    proj.direction      = { std::cos(jet.heading), -0.1f, std::sin(jet.heading) };
    proj.direction      = proj.direction.normalised();
    proj.muzzleVelocity = 340.f;
    proj.mass           = 210.f;
    proj.dragCoefficient= 0.4f;
    proj.crossSection   = 0.01f;
    proj.simulationTime = 60.f;
    outProjectiles.push_back(proj);
    std::cout << "[Jet] " << jet.desc.name << " deployed weapon\n";
}

// ===========================================================================
// VehicleSystem
// ===========================================================================

bool VehicleSystem::initialize() {
    std::cout << "[VehicleSystem] Initialized\n";
    return true;
}

void VehicleSystem::shutdown() {
    m_vehicles.clear();
    std::cout << "[VehicleSystem] Shutdown\n";
}

EntityID VehicleSystem::spawnVehicle(ECSWorld& world, const VehicleDesc& desc,
                                      Vec3 position) {
    EntityID id = world.createEntity();

    TransformComponent tf;
    tf.x = position.x; tf.y = position.y; tf.z = position.z;
    world.addComponent<TransformComponent>(id, tf);
    world.addComponent<VelocityComponent>(id);
    world.addComponent<HealthComponent>(id, {100.f, 100.f, 0.f});
    world.addComponent<RenderableComponent>(id);

    VehicleInstance inst;
    inst.entityID = id;
    inst.desc     = desc;
    inst.position = position;
    inst.fuel     = desc.maxFuelLitres;
    inst.health   = 100.f;
    m_vehicles[id] = std::move(inst);

    std::cout << "[VehicleSystem] Spawned " << desc.name << " (entity=" << id << ")\n";
    return id;
}

void VehicleSystem::despawnVehicle(ECSWorld& world, EntityID id) {
    world.destroyEntity(id);
    m_vehicles.erase(id);
}

bool VehicleSystem::boardVehicle(EntityID vehicleID, EntityID passengerID) {
    auto it = m_vehicles.find(vehicleID);
    if (it == m_vehicles.end()) return false;
    VehicleInstance& v = it->second;
    if (static_cast<u32>(v.occupants.size()) >= v.desc.maxOccupants) return false;

    VehicleOccupant occ;
    occ.entityID = passengerID;
    occ.isDriver = v.occupants.empty(); // first to board is driver
    v.occupants.push_back(occ);
    return true;
}

bool VehicleSystem::leaveVehicle(EntityID vehicleID, EntityID passengerID) {
    auto it = m_vehicles.find(vehicleID);
    if (it == m_vehicles.end()) return false;
    auto& occ = it->second.occupants;
    auto oit = std::find_if(occ.begin(), occ.end(),
        [passengerID](const VehicleOccupant& o){ return o.entityID == passengerID; });
    if (oit == occ.end()) return false;
    occ.erase(oit);
    return true;
}

VehicleInstance* VehicleSystem::getVehicle(EntityID id) {
    auto it = m_vehicles.find(id);
    return it != m_vehicles.end() ? &it->second : nullptr;
}

void VehicleSystem::update(ECSWorld& world, f32 dt) {
    std::vector<ProjectileDesc> projectiles; // accumulated this frame

    for (auto& [eid, inst] : m_vehicles) {
        if (inst.state == VehicleState::Destroyed) continue;

        switch (inst.desc.type) {
            case VehicleType::Tank:
            case VehicleType::IFV:
                // AI-driven or player throttle/steer would come from outside.
                // For demo, idle update.
                m_tankCtrl.update(inst, 0.f, 0.f, dt);
                break;
            case VehicleType::Helicopter:
            case VehicleType::AttackHeli:
                m_heliCtrl.update(inst, 0.f, 0.f, 0.f, 0.f, dt);
                break;
            case VehicleType::Jet:
            case VehicleType::CAS:
                m_jetCtrl.update(inst, 0.f, 0.f, 0.f, 0.f, dt);
                break;
            default:
                break;
        }

        // Sync transform back to ECS.
        if (auto* tf = world.getComponent<TransformComponent>(eid)) {
            tf->x = inst.position.x;
            tf->y = inst.position.y;
            tf->z = inst.position.z;
        }
        if (auto* vl = world.getComponent<VelocityComponent>(eid)) {
            vl->vx = inst.velocity.x;
            vl->vy = inst.velocity.y;
            vl->vz = inst.velocity.z;
        }
    }
}

} // namespace shooter
