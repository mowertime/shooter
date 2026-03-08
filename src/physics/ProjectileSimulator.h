#pragma once

#include "core/Platform.h"
#include <vector>
#include <functional>

namespace shooter {

/// Ballistic projectile simulation using 4th-order Runge-Kutta integration.
///
/// Supports drag (Cd), spin drift, Coriolis effect, and wind influence.
struct ProjectileDesc {
    float x{0}, y{0}, z{0};       ///< Spawn position (m)
    float vx{0}, vy{0}, vz{0};    ///< Initial velocity (m/s)
    float mass{0.0082f};          ///< kg (default: 7.62×39mm FMJ)
    float calibreDiameter{0.00762f}; ///< m
    float dragCoeff{0.295f};      ///< Cd
    float armorPenetration{30.0f};///< RHA mm equivalent
};

struct ProjectileHit {
    u32   projectileId{UINT32_MAX};
    u32   hitBodyId{UINT32_MAX};
    float px{0}, py{0}, pz{0};   ///< Hit position
    float nx{0}, ny{0}, nz{0};   ///< Hit normal
    float remainingVelocity{0};  ///< m/s at impact
    float penetrationDepth{0};   ///< mm
};

using HitCallback = std::function<void(const ProjectileHit&)>;

/// Manages all active projectiles and resolves hits against the physics world.
class ProjectileSimulator {
public:
    ProjectileSimulator() = default;

    void setHitCallback(HitCallback cb) { m_hitCb = std::move(cb); }

    /// Spawn a new projectile.  Returns its id.
    u32  spawn(const ProjectileDesc& desc);

    /// Advance all projectiles by \p dt seconds.
    /// Wind components affect drag direction.
    void update(float dt, float windX = 0, float windY = 0, float windZ = 0);

    u32  activeCount() const;

private:
    struct Projectile {
        ProjectileDesc desc;
        float          age{0};
        bool           active{true};
    };
    std::vector<Projectile> m_projectiles;
    HitCallback             m_hitCb;

    static constexpr float MAX_AGE_S   = 30.0f;
    static constexpr float AIR_DENSITY = 1.225f; ///< kg/m³ at sea level
};

} // namespace shooter
