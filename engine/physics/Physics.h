#pragma once

// ---------------------------------------------------------------------------
// Physics.h — Physics world interface.
//
// Wraps rigid bodies, soft bodies, vehicles, aircraft, and a custom ballistics
// solver that accounts for drag, wind, and gravity.  A production build would
// delegate to PhysX / Bullet; this prototype owns a minimal implementation.
// ---------------------------------------------------------------------------

#include <vector>
#include <unordered_map>
#include <array>
#include <cmath>

#include "engine/platform/Platform.h"
#include "engine/ecs/ECS.h"

namespace shooter {

// ---- Vector3 helper (avoids pulling in a math library for the prototype) ---
struct Vec3 {
    f32 x{0}, y{0}, z{0};

    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3 operator*(f32 s)         const { return {x*s,   y*s,   z*s};   }
    Vec3& operator+=(const Vec3& o) { x+=o.x; y+=o.y; z+=o.z; return *this; }

    [[nodiscard]] f32 length()   const { return std::sqrt(x*x + y*y + z*z); }
    [[nodiscard]] Vec3 normalised() const {
        f32 len = length();
        return len > 1e-6f ? Vec3{x/len, y/len, z/len} : Vec3{};
    }
    [[nodiscard]] f32 dot(const Vec3& o) const { return x*o.x + y*o.y + z*o.z; }
};

// ---- Collision shapes ------------------------------------------------------
enum class PhysicsShape { Sphere, Box, Capsule, ConvexMesh, HeightField };

struct ShapeDesc {
    PhysicsShape shape{PhysicsShape::Sphere};
    f32          radius{0.5f};          ///< sphere / capsule radius
    Vec3         halfExtents{1,1,1};    ///< box half-size
    f32          capsuleHalfHeight{1.f};
};

// ---- Rigid body ------------------------------------------------------------
struct RigidBodyDesc {
    EntityID  entity{INVALID_ENTITY};
    f32       mass{1.f};       ///< 0 = static (infinite mass)
    ShapeDesc shape;
    Vec3      initialPosition;
    bool      isKinematic{false};
};

struct RigidBody {
    EntityID entity{INVALID_ENTITY};
    Vec3     position;
    Vec3     velocity;
    Vec3     angularVelocity;
    f32      mass{1.f};
    f32      inverseMass{1.f};
    bool     isStatic{false};
    bool     isKinematic{false};
    ShapeDesc shape;
};

// ---- Vehicle ---------------------------------------------------------------
struct VehicleConfig {
    f32  mass{8000.f};         ///< kg (tank ~60 000 kg)
    f32  engineTorque{3000.f}; ///< N·m
    f32  maxSpeed{15.f};       ///< m/s
    f32  wheelRadius{0.4f};
    u32  wheelCount{4};
    f32  suspensionStiffness{40000.f};
};

// ---- Aircraft --------------------------------------------------------------
struct AircraftConfig {
    f32  mass{9000.f};        ///< kg
    f32  thrustMax{50000.f};  ///< N
    f32  liftCoefficient{1.2f};
    f32  dragCoefficient{0.3f};
    f32  wingArea{16.f};      ///< m²
};

// ---- Ballistic projectile --------------------------------------------------
struct ProjectileDesc {
    Vec3  origin;
    Vec3  direction;       ///< normalised initial direction
    f32   muzzleVelocity;  ///< m/s
    f32   mass;            ///< kg
    f32   dragCoefficient; ///< Cd
    f32   crossSection;    ///< m² (affects drag)
    f32   simulationTime;  ///< seconds to trace
    Vec3  wind;            ///< ambient wind vector
};

/// One sample point on a ballistic trajectory.
struct TrajectoryPoint {
    Vec3 position;
    Vec3 velocity;
    f32  time{0};
};

// ===========================================================================
// PhysicsWorld
// ===========================================================================
class PhysicsWorld {
public:
    static constexpr f32 GRAVITY = -9.81f;

    PhysicsWorld()  = default;
    ~PhysicsWorld() = default;

    bool initialize();
    void shutdown();

    // ---- Body management ---------------------------------------------------
    void addRigidBody(const RigidBodyDesc& desc);
    void addSoftBody(EntityID entity);
    void addVehicle(EntityID entity, const VehicleConfig& config);
    void addAircraft(EntityID entity, const AircraftConfig& config);

    void removeBody(EntityID entity);

    // ---- Queries -----------------------------------------------------------
    [[nodiscard]] Vec3 getPosition(EntityID entity) const;
    [[nodiscard]] Vec3 getVelocity(EntityID entity) const;
    void setPosition(EntityID entity, Vec3 pos);
    void applyImpulse(EntityID entity, Vec3 impulse);

    // ---- Ballistics --------------------------------------------------------
    /// Simulate a projectile in flight; returns sampled trajectory points.
    std::vector<TrajectoryPoint> simulateProjectile(const ProjectileDesc& desc);

    // ---- Simulation step ---------------------------------------------------
    void step(f32 dt);

private:
    // Simple Euler integration for rigid bodies (production: Runge-Kutta 4).
    void integrateRigidBodies(f32 dt);

    // Broad-phase: axis-aligned bounding-box sweep-and-prune.
    void broadPhase();

    // Narrow-phase: GJK collision detection (stub).
    void narrowPhase();

    // Constraint solver: iterative impulse method (PGS, stub).
    void solveConstraints(f32 dt);

    std::unordered_map<EntityID, RigidBody>      m_rigidBodies;
    std::unordered_map<EntityID, VehicleConfig>  m_vehicles;
    std::unordered_map<EntityID, AircraftConfig> m_aircraft;

    // Air density used for drag calculation (kg/m³, sea level standard).
    static constexpr f32 AIR_DENSITY = 1.225f;
};

} // namespace shooter
