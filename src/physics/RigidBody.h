#pragma once

#include "core/Platform.h"
#include <cmath>

namespace shooter {

/// Rigid-body handle – thin wrapper around a physics-body id.
struct RigidBody {
    u32 bodyId{UINT32_MAX};
    bool isValid() const { return bodyId != UINT32_MAX; }
};

/// Extended vehicle contact info used by the vehicle physics solver.
struct WheelContactInfo {
    bool  onGround{false};
    float suspensionLength{0.0f};
    float lateralForce{0.0f};
    float longitudinalForce{0.0f};
};

/// Wheeled vehicle physics parameters.
struct VehicleParams {
    float mass{3000.0f};            ///< kg
    float wheelBase{2.8f};          ///< m
    float trackWidth{1.8f};         ///< m
    float maxSteerAngle{0.55f};     ///< radians
    float maxEngineTorque{3000.0f}; ///< Nm
    float brakeTorque{8000.0f};     ///< Nm
    float dragCoeff{0.35f};
    u32   numWheels{4};
};

/// Fixed-wing / rotary aircraft physics parameters.
struct AircraftParams {
    float mass{8000.0f};            ///< kg
    float wingArea{30.0f};          ///< m²
    float liftCoeffBase{1.2f};
    float dragCoeffBase{0.04f};
    float maxThrust{40000.0f};      ///< N
    bool  isRotary{false};          ///< true = helicopter, false = fixed-wing
};

} // namespace shooter
