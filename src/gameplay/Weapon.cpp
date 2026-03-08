#include "gameplay/Weapon.h"
#include <algorithm>
#include <cmath>

namespace shooter {

// ── Weapon ────────────────────────────────────────────────────────────────────

void Weapon::removeAttachment(AttachmentType type) {
    m_attachments.erase(
        std::remove_if(m_attachments.begin(), m_attachments.end(),
                       [type](const Attachment& a) { return a.type == type; }),
        m_attachments.end());
}

float Weapon::effectiveMuzzleVelocity() const {
    float mv = m_profile.muzzleVelocity;
    for (const auto& a : m_attachments) {
        if (a.type == AttachmentType::Suppressor) mv *= 0.95f;
    }
    return mv;
}

float Weapon::effectiveFirerateHz() const {
    return m_profile.firerate / 60.0f;
}

bool Weapon::fire(ProjectileSimulator& sim,
                  float ox, float oy, float oz,
                  float dx, float dy, float dz,
                  i32& ammoInMag) {
    if (ammoInMag <= 0) return false;
    --ammoInMag;

    float mv = effectiveMuzzleVelocity();
    // Compute combined recoil modifier from attachments.
    float recoilMult = 1.0f;
    for (const auto& a : m_attachments) recoilMult *= a.recoilMult;
    (void)recoilMult; // Applied to camera in Player.

    float len = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (len < 1e-6f) { dx = 0; dy = 0; dz = 1; len = 1; }

    ProjectileDesc pd;
    pd.x  = ox; pd.y  = oy; pd.z  = oz;
    pd.vx = (dx / len) * mv;
    pd.vy = (dy / len) * mv;
    pd.vz = (dz / len) * mv;
    pd.mass              = m_profile.mass;
    pd.calibreDiameter   = m_profile.calibre;
    pd.dragCoeff         = m_profile.dragCoeff;
    pd.armorPenetration  = m_profile.armorPenetration;
    sim.spawn(pd);
    return true;
}

// ── WeaponProfiles ────────────────────────────────────────────────────────────

namespace WeaponProfiles {

WeaponProfile ak47() {
    WeaponProfile p;
    p.name             = "AK-47";
    p.muzzleVelocity   = 715.0f;
    p.firerate         = 600.0f;
    p.recoilVert       = 0.4f;
    p.mass             = 0.0082f;
    p.calibre          = 0.00762f;
    p.dragCoeff        = 0.295f;
    p.armorPenetration = 20.0f;
    p.magCapacity      = 30;
    p.fullyAutomatic   = true;
    return p;
}

WeaponProfile m4a1() {
    WeaponProfile p;
    p.name             = "M4A1";
    p.muzzleVelocity   = 910.0f;
    p.firerate         = 800.0f;
    p.recoilVert       = 0.3f;
    p.mass             = 0.004f;
    p.calibre          = 0.00556f;
    p.dragCoeff        = 0.28f;
    p.armorPenetration = 15.0f;
    p.magCapacity      = 30;
    p.fullyAutomatic   = true;
    return p;
}

WeaponProfile m82Barrett() {
    WeaponProfile p;
    p.name             = "M82 Barrett";
    p.muzzleVelocity   = 853.0f;
    p.firerate         = 60.0f;
    p.recoilVert       = 1.2f;
    p.mass             = 0.0429f;
    p.calibre          = 0.0127f;
    p.dragCoeff        = 0.25f;
    p.armorPenetration = 38.0f;
    p.magCapacity      = 10;
    p.fullyAutomatic   = false;
    return p;
}

WeaponProfile glock17() {
    WeaponProfile p;
    p.name             = "Glock 17";
    p.muzzleVelocity   = 375.0f;
    p.firerate         = 400.0f;
    p.recoilVert       = 0.2f;
    p.mass             = 0.008f;
    p.calibre          = 0.009f;
    p.dragCoeff        = 0.32f;
    p.armorPenetration = 5.0f;
    p.magCapacity      = 17;
    p.fullyAutomatic   = false;
    return p;
}

WeaponProfile m249Saw() {
    WeaponProfile p;
    p.name             = "M249 SAW";
    p.muzzleVelocity   = 915.0f;
    p.firerate         = 800.0f;
    p.recoilVert       = 0.35f;
    p.mass             = 0.004f;
    p.calibre          = 0.00556f;
    p.dragCoeff        = 0.28f;
    p.armorPenetration = 15.0f;
    p.magCapacity      = 200;
    p.fullyAutomatic   = true;
    return p;
}

WeaponProfile rocketLauncher() {
    WeaponProfile p;
    p.name             = "RPG-7";
    p.muzzleVelocity   = 295.0f;
    p.firerate         = 6.0f;
    p.recoilVert       = 2.0f;
    p.mass             = 2.25f;
    p.calibre          = 0.085f;
    p.dragCoeff        = 0.4f;
    p.armorPenetration = 500.0f;
    p.magCapacity      = 1;
    p.fullyAutomatic   = false;
    return p;
}

} // namespace WeaponProfiles
} // namespace shooter
