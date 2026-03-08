#include "physics/ProjectileSimulator.h"
#include <cmath>
#include <numbers>

namespace shooter {

u32 ProjectileSimulator::spawn(const ProjectileDesc& desc) {
    u32 id = static_cast<u32>(m_projectiles.size());
    m_projectiles.push_back({desc, 0.0f, true});
    return id;
}

void ProjectileSimulator::update(float dt, float windX, float windY, float windZ) {
    for (auto& p : m_projectiles) {
        if (!p.active) continue;
        p.age += dt;
        if (p.age > MAX_AGE_S) { p.active = false; continue; }

        ProjectileDesc& d = p.desc;

        // Compute relative velocity against wind.
        float rvx = d.vx - windX;
        float rvy = d.vy - windY;
        float rvz = d.vz - windZ;
        float spd = std::sqrt(rvx*rvx + rvy*rvy + rvz*rvz);
        if (spd < 1e-4f) spd = 1e-4f;

        // Cross-sectional area.
        float radius = d.calibreDiameter * 0.5f;
        float area   = std::numbers::pi_v<float> * radius * radius;

        // Drag force magnitude: F_d = 0.5 * rho * v² * Cd * A
        float dragMag = 0.5f * AIR_DENSITY * spd * spd * d.dragCoeff * area;
        float invMass = 1.0f / d.mass;
        float dragAcc = dragMag * invMass;

        // Deceleration along velocity direction.
        d.vx -= (rvx / spd) * dragAcc * dt;
        d.vy -= (rvy / spd) * dragAcc * dt;
        d.vz -= (rvz / spd) * dragAcc * dt;

        // Gravity.
        d.vy -= 9.81f * dt;

        // Integrate position (Euler – sufficient for small dt).
        d.x += d.vx * dt;
        d.y += d.vy * dt;
        d.z += d.vz * dt;

        // Ground check.
        if (d.y < 0.0f) {
            p.active = false;
            if (m_hitCb) {
                ProjectileHit hit;
                hit.projectileId       = static_cast<u32>(&p - m_projectiles.data());
                hit.px = d.x; hit.py = 0; hit.pz = d.z;
                hit.nx = 0; hit.ny = 1; hit.nz = 0;
                hit.remainingVelocity  = spd;
                m_hitCb(hit);
            }
        }
    }
}

u32 ProjectileSimulator::activeCount() const {
    u32 n = 0;
    for (const auto& p : m_projectiles) n += p.active ? 1 : 0;
    return n;
}

} // namespace shooter
