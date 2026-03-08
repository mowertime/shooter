#pragma once

#include "ecs/Entity.h"
#include "ecs/Component.h"

namespace shooter {

class EntityRegistry;

/// Base class for all ECS systems.
///
/// Each system declares which component types it requires via its
/// \c signature() mask.  The registry automatically calls \c update()
/// for every entity that satisfies the signature.
class ISystem {
public:
    virtual ~ISystem() = default;

    /// The component mask this system cares about.
    virtual ComponentMask signature() const = 0;

    /// Per-frame update.  \p dt is the timestep in seconds.
    virtual void update(EntityRegistry& registry, float dt) = 0;
};

/// Physics integration system.
class PhysicsIntegrationSystem final : public ISystem {
public:
    ComponentMask signature() const override;
    void update(EntityRegistry& registry, float dt) override;
};

/// AI decision-making system.
class AIUpdateSystem final : public ISystem {
public:
    ComponentMask signature() const override;
    void update(EntityRegistry& registry, float dt) override;
};

/// Weapon cooldown / reload ticker.
class WeaponSystem final : public ISystem {
public:
    ComponentMask signature() const override;
    void update(EntityRegistry& registry, float dt) override;
};

} // namespace shooter
