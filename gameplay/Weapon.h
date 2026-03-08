#pragma once

// ---------------------------------------------------------------------------
// Weapon.h — Modular weapon system with realistic ballistics.
// ---------------------------------------------------------------------------

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include "engine/platform/Platform.h"
#include "engine/physics/Physics.h"  // Vec3, ProjectileDesc

namespace shooter {

// ---------------------------------------------------------------------------
// Weapon types
// ---------------------------------------------------------------------------
enum class WeaponType {
    Pistol,
    SMG,
    AssaultRifle,
    BattleRifle,
    Sniper,
    Shotgun,
    LMG,
    RPG,
    GrenadeLauncher,
    AntiTankMissile,
};

// ---------------------------------------------------------------------------
// Attachment slots
// ---------------------------------------------------------------------------
enum class AttachmentSlot {
    Optic,
    Muzzle,       ///< suppressor, flash hider, muzzle brake
    UnderBarrel,  ///< grip, bipod, grenade launcher
    Magazine,
    Stock,
};

struct WeaponAttachment {
    AttachmentSlot slot;
    std::string    name;

    // Stat modifiers (additive on top of base weapon stats)
    f32  damageMultiplier{1.f};
    f32  recoilMultiplier{1.f};
    f32  spreadMultiplier{1.f};
    f32  muzzleVelocityAdd{0.f};  ///< m/s
    bool suppressesFire{false};   ///< suppressor
    f32  magnification{1.f};      ///< optic zoom
};

// ---------------------------------------------------------------------------
// ArmorPenetration — computes residual damage after armour.
// ---------------------------------------------------------------------------
struct ArmorPenetration {
    f32 penetrationMm{10.f};  ///< RHA equivalent mm
    f32 velocityRequired{400.f}; ///< m/s to achieve full penetration

    /// Returns the fraction of damage that passes through armour [0,1].
    [[nodiscard]] f32 evaluate(f32 bulletVelocity, f32 armourThicknessMm) const {
        if (armourThicknessMm <= 0.f) return 1.f;
        f32 velFactor = bulletVelocity / velocityRequired;
        f32 penRatio  = penetrationMm / armourThicknessMm * velFactor;
        // Sigmoidal transition: full pen → partial pen → ricochet.
        if (penRatio >= 1.f) return 1.f;
        if (penRatio < 0.1f) return 0.f;
        return penRatio * penRatio; // quadratic falloff
    }
};

// ---------------------------------------------------------------------------
// WeaponStats — base properties of a weapon (before attachments).
// ---------------------------------------------------------------------------
struct WeaponStats {
    WeaponType  type{WeaponType::AssaultRifle};
    std::string name;

    // Ballistics
    f32 muzzleVelocity{900.f};    ///< m/s
    f32 bulletMass{0.004f};       ///< kg (4g for 5.56)
    f32 bulletDrag{0.3f};         ///< drag coefficient
    f32 bulletCrossSection{2e-5f};///< m²
    f32 damage{35.f};             ///< base hit damage (no armour)
    ArmorPenetration penetration;

    // Handling
    f32 rpm{750.f};           ///< rounds per minute (cyclic rate)
    f32 horizontalRecoil{1.f};///< degrees
    f32 verticalRecoil{2.5f};
    f32 hipSpread{3.f};       ///< degrees
    f32 adsSpread{0.5f};

    // Ammunition
    i32 magazineSize{30};
    i32 reserveAmmo{120};
    f32 reloadTime{2.5f};     ///< seconds

    bool isSemiAuto{false};
    bool isBoltAction{false};
};

// ===========================================================================
// WeaponSystem — manages the lifecycle of a single equipped weapon.
// ===========================================================================
class WeaponSystem {
public:
    void equip(const WeaponStats& stats);
    void addAttachment(WeaponAttachment attachment);
    void removeAttachment(AttachmentSlot slot);

    /// Attempt to fire; returns true if a shot was fired.
    bool fire(Vec3 muzzlePos, Vec3 direction, Vec3 wind,
              std::vector<ProjectileDesc>& outProjectiles);

    void beginReload();
    void update(f32 dt);

    [[nodiscard]] bool         isADS()            const { return m_isADS; }
    void                       setADS(bool ads)         { m_isADS = ads; }
    [[nodiscard]] bool         isReloading()      const { return m_reloadTimer > 0.f; }
    [[nodiscard]] i32          ammoInMag()        const { return m_ammoInMag; }
    [[nodiscard]] i32          reserveAmmo()      const { return m_reserveAmmo; }
    [[nodiscard]] const WeaponStats& stats()      const { return m_stats; }

    /// Compute the effective spread angle in degrees accounting for attachments.
    [[nodiscard]] f32 effectiveSpread() const;

private:
    WeaponStats   m_stats;
    bool          m_isADS{false};
    i32           m_ammoInMag{0};
    i32           m_reserveAmmo{0};
    f32           m_fireCooldown{0.f};  ///< seconds until next shot allowed
    f32           m_reloadTimer{0.f};

    std::unordered_map<AttachmentSlot, WeaponAttachment> m_attachments;
};

} // namespace shooter
