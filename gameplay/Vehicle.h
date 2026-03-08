#pragma once

// ---------------------------------------------------------------------------
// Vehicle.h — Combined-arms vehicle system.
// ---------------------------------------------------------------------------

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

#include "engine/platform/Platform.h"
#include "engine/physics/Physics.h"   // Vec3, VehicleConfig
#include "engine/ecs/ECS.h"

namespace shooter {

// ---------------------------------------------------------------------------
// VehicleType
// ---------------------------------------------------------------------------
enum class VehicleType {
    Tank,
    APC,
    IFV,          ///< Infantry Fighting Vehicle
    Truck,
    Helicopter,
    AttackHeli,
    Jet,
    CAS,          ///< Close Air Support
    Boat,
    Submarine,
};

// ---------------------------------------------------------------------------
// VehicleState
// ---------------------------------------------------------------------------
enum class VehicleState {
    Idle,
    Moving,
    Firing,
    Disabled,   ///< mobility kill
    Destroyed,
};

// ---------------------------------------------------------------------------
// VehicleOccupant — one crew position (driver, gunner, commander, …)
// ---------------------------------------------------------------------------
struct VehicleOccupant {
    EntityID entityID{INVALID_ENTITY};
    std::string seatName;
    bool isDriver{false};
    bool isGunner{false};
};

// ---------------------------------------------------------------------------
// VehicleDesc — template for one vehicle type.
// ---------------------------------------------------------------------------
struct VehicleDesc {
    VehicleType type{VehicleType::Tank};
    std::string name;
    VehicleConfig physicsConfig;
    u32  maxOccupants{4};
    f32  armourThicknessFront{400.f};  ///< mm RHA
    f32  armourThicknessSide{100.f};
    f32  armourThicknessRear{50.f};
    f32  mainGunCalibremm{120.f};
    f32  maxFuelLitres{1000.f};
    f32  fuelConsumptionRate{0.5f};    ///< litres/km
};

// ===========================================================================
// VehicleInstance — runtime state of a spawned vehicle.
// ===========================================================================
struct VehicleInstance {
    EntityID     entityID{INVALID_ENTITY};
    VehicleDesc  desc;
    VehicleState state{VehicleState::Idle};

    Vec3 position{};
    Vec3 velocity{};
    f32  heading{0.f};    ///< yaw radians
    f32  fuel{0.f};
    f32  health{100.f};

    std::vector<VehicleOccupant> occupants;
    bool hasDriver() const {
        for (auto& o : occupants) if (o.isDriver) return true;
        return false;
    }
};

// ===========================================================================
// TankController — special logic for tracked vehicles.
// ===========================================================================
class TankController {
public:
    void update(VehicleInstance& tank, f32 throttle, f32 steer, f32 dt);
    void firePrimaryWeapon(VehicleInstance& tank,
                           std::vector<ProjectileDesc>& outProjectiles);

private:
    f32 m_gunCooldown{0.f};
    f32 m_turretYaw{0.f};
};

// ===========================================================================
// HelicopterController — lift + cyclic / collective model.
// ===========================================================================
class HelicopterController {
public:
    void update(VehicleInstance& heli,
                f32 collective,    ///< [-1, 1] main rotor lift
                f32 cyclicX,       ///< pitch tilt
                f32 cyclicZ,       ///< roll tilt
                f32 pedal,         ///< yaw (tail rotor)
                f32 dt);
private:
    f32 m_rotorRPM{0.f};
};

// ===========================================================================
// JetController — simplified fixed-wing flight model.
// ===========================================================================
class JetController {
public:
    void update(VehicleInstance& jet,
                f32 throttle,
                f32 pitch, f32 roll, f32 yaw,
                f32 dt);
    void deployWeapon(VehicleInstance& jet,
                      std::vector<ProjectileDesc>& outProjectiles);
private:
    f32 m_afterburnerFuel{100.f};
};

// ===========================================================================
// VehicleSystem — manages all spawned vehicles.
// ===========================================================================
class VehicleSystem {
public:
    bool initialize();
    void shutdown();
    void update(ECSWorld& world, f32 dt);

    EntityID spawnVehicle(ECSWorld& world, const VehicleDesc& desc,
                          Vec3 position);
    void     despawnVehicle(ECSWorld& world, EntityID id);

    bool boardVehicle(EntityID vehicleID, EntityID passengerID);
    bool leaveVehicle(EntityID vehicleID, EntityID passengerID);

    [[nodiscard]] VehicleInstance* getVehicle(EntityID id);
    [[nodiscard]] u32 vehicleCount() const {
        return static_cast<u32>(m_vehicles.size());
    }

private:
    std::unordered_map<EntityID, VehicleInstance> m_vehicles;
    TankController       m_tankCtrl;
    HelicopterController m_heliCtrl;
    JetController        m_jetCtrl;
};

} // namespace shooter
