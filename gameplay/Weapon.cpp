// ---------------------------------------------------------------------------
// Weapon.cpp — WeaponSystem implementation.
// ---------------------------------------------------------------------------

#include "gameplay/Weapon.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <numbers>  // std::numbers::pi_v (C++20)

namespace shooter {

static f32 degToRad(f32 deg) {
    return deg * static_cast<f32>(std::numbers::pi_v<f64>) / 180.f;
}

void WeaponSystem::equip(const WeaponStats& stats) {
    m_stats       = stats;
    m_ammoInMag   = stats.magazineSize;
    m_reserveAmmo = stats.reserveAmmo;
    m_fireCooldown= 0.f;
    m_reloadTimer = 0.f;
    m_attachments.clear();
    std::cout << "[Weapon] Equipped: " << stats.name << "\n";
}

void WeaponSystem::addAttachment(WeaponAttachment attachment) {
    m_attachments[attachment.slot] = std::move(attachment);
}

void WeaponSystem::removeAttachment(AttachmentSlot slot) {
    m_attachments.erase(slot);
}

void WeaponSystem::update(f32 dt) {
    // Advance fire rate cooldown.
    if (m_fireCooldown > 0.f)
        m_fireCooldown = std::max(0.f, m_fireCooldown - dt);

    // Complete reload.
    if (m_reloadTimer > 0.f) {
        m_reloadTimer -= dt;
        if (m_reloadTimer <= 0.f) {
            m_reloadTimer = 0.f;
            i32 needed  = m_stats.magazineSize - m_ammoInMag;
            i32 taken   = std::min(needed, m_reserveAmmo);
            m_ammoInMag   += taken;
            m_reserveAmmo -= taken;
            std::cout << "[Weapon] Reload complete. Mag: " << m_ammoInMag
                      << " / Reserve: " << m_reserveAmmo << "\n";
        }
    }
}

bool WeaponSystem::fire(Vec3 muzzlePos, Vec3 direction, Vec3 wind,
                        std::vector<ProjectileDesc>& outProjectiles) {
    if (m_fireCooldown > 0.f)  return false;
    if (m_reloadTimer  > 0.f)  return false;
    if (m_ammoInMag    <= 0)   { beginReload(); return false; }

    // Consume one round.
    --m_ammoInMag;

    // Effective muzzle velocity (attachments may add/subtract).
    f32 mv = m_stats.muzzleVelocity;
    for (auto& [slot, att] : m_attachments) {
        mv += att.muzzleVelocityAdd;
    }

    // Apply spread: deflect direction by a random angle within the spread cone.
    // For a prototype we use a fixed small deflection; production = box-Muller.
    f32 spreadDeg = effectiveSpread();
    f32 spreadRad = degToRad(spreadDeg);
    // Tiny deterministic offset for demo purposes.
    Vec3 fired = {
        direction.x + spreadRad * 0.1f,
        direction.y,
        direction.z + spreadRad * 0.1f,
    };
    fired = fired.normalised();

    // Build projectile descriptor.
    ProjectileDesc proj;
    proj.origin         = muzzlePos;
    proj.direction      = fired;
    proj.muzzleVelocity = mv;
    proj.mass           = m_stats.bulletMass;
    proj.dragCoefficient= m_stats.bulletDrag;
    proj.crossSection   = m_stats.bulletCrossSection;
    proj.simulationTime = 8.f; // simulate up to 8 s of flight
    proj.wind           = wind;
    outProjectiles.push_back(proj);

    // Set fire-rate cooldown: seconds per round = 60 / rpm.
    m_fireCooldown = 60.f / m_stats.rpm;

    if (m_stats.isBoltAction) m_fireCooldown = m_stats.reloadTime;

    return true;
}

void WeaponSystem::beginReload() {
    if (m_reloadTimer > 0.f) return;
    if (m_ammoInMag == m_stats.magazineSize) return;
    if (m_reserveAmmo <= 0) { std::cout << "[Weapon] No reserve ammo!\n"; return; }

    m_reloadTimer = m_stats.reloadTime;
    std::cout << "[Weapon] Reloading...\n";
}

f32 WeaponSystem::effectiveSpread() const {
    f32 base = m_isADS ? m_stats.adsSpread : m_stats.hipSpread;
    for (auto& [slot, att] : m_attachments) {
        base *= att.spreadMultiplier;
    }
    return base;
}

} // namespace shooter
