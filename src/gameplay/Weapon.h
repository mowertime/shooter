#pragma once

#include "ecs/EntityRegistry.h"
#include "physics/ProjectileSimulator.h"
#include <string>
#include <vector>

namespace shooter {

/// Weapon attachment types.
enum class AttachmentType : u8 {
    Optic,
    Suppressor,
    ForwardGrip,
    Magazine,
    Light,
    Laser,
};

struct Attachment {
    AttachmentType type;
    u32            assetId{UINT32_MAX};
    float          recoilMult{1.0f};
    float          noiseMult{1.0f};
    float          rangeMult{1.0f};
};

/// Ballistic profile for a weapon type.
struct WeaponProfile {
    std::string name;
    float muzzleVelocity{900.0f};    ///< m/s
    float firerate{600.0f};          ///< rounds/min
    float recoilVert{0.3f};          ///< radians per shot
    float recoilHoriz{0.05f};
    float mass{0.008f};              ///< Projectile mass (kg)
    float calibre{0.00556f};         ///< Projectile diameter (m)
    float dragCoeff{0.295f};
    float armorPenetration{15.0f};   ///< RHA mm
    i32   magCapacity{30};
    bool  fullyAutomatic{true};
};

/// Modular weapon system.
///
/// A weapon combines a base profile with up to 6 attachments.
/// Ballistic properties are derived from the combined stats at fire time.
class Weapon {
public:
    Weapon() = default;
    explicit Weapon(const WeaponProfile& profile) : m_profile(profile) {}

    void addAttachment(const Attachment& a) {
        if (m_attachments.size() < 6) m_attachments.push_back(a);
    }

    void removeAttachment(AttachmentType type);

    /// Fire toward (dx, dy, dz) from world position (ox, oy, oz).
    /// Spawns a projectile in the simulator.
    bool fire(ProjectileSimulator& sim,
              float ox, float oy, float oz,
              float dx, float dy, float dz,
              i32& ammoInMag);

    const WeaponProfile& profile() const { return m_profile; }

    float effectiveMuzzleVelocity() const;
    float effectiveFirerateHz()     const;

private:
    WeaponProfile            m_profile;
    std::vector<Attachment>  m_attachments;
};

/// Library of pre-defined weapon profiles.
namespace WeaponProfiles {
    WeaponProfile ak47();
    WeaponProfile m4a1();
    WeaponProfile m82Barrett();
    WeaponProfile glock17();
    WeaponProfile m249Saw();
    WeaponProfile rocketLauncher();
}

} // namespace shooter
