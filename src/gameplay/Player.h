#pragma once

#include "ecs/EntityRegistry.h"
#include "physics/PhysicsWorld.h"
#include "physics/ProjectileSimulator.h"

namespace shooter {

/// Player movement state flags.
enum class MoveState : u8 {
    Standing = 0,
    Crouching,
    Prone,
    Vaulting,
    Climbing,
};

/// Player input snapshot (collected each frame).
struct PlayerInput {
    float moveForward{0};   ///< -1..1
    float moveSide{0};      ///< -1..1
    float lookYaw{0};       ///< radians/frame delta
    float lookPitch{0};
    bool  jump{false};
    bool  crouch{false};
    bool  prone{false};
    bool  leanLeft{false};
    bool  leanRight{false};
    bool  fire{false};
    bool  aimDownSights{false};
    bool  reload{false};
    bool  vault{false};
    bool  interact{false};
};

/// First-person player controller.
///
/// Handles advanced movement (vaulting, climbing, prone, leaning),
/// health, and weapon management.
class Player {
public:
    explicit Player(EntityRegistry& registry, PhysicsWorld& physics);
    ~Player();

    void update(const PlayerInput& input, float dt);

    Entity    entity()    const { return m_entity; }
    MoveState moveState() const { return m_moveState; }
    float     health()    const;
    bool      isAlive()   const;

    void setPosition(float x, float y, float z);
    void getPosition(float& x, float& y, float& z) const;
    float getYaw()   const { return m_yaw;   }
    float getPitch() const { return m_pitch; }

private:
    void handleMovement(const PlayerInput& input, float dt);
    void handleLook    (const PlayerInput& input, float dt);
    void handleVault   (float dt);

    EntityRegistry& m_registry;
    PhysicsWorld&   m_physics;
    Entity          m_entity;
    u32             m_physicsBodyId{UINT32_MAX};

    float     m_yaw{0}, m_pitch{0};
    MoveState m_moveState{MoveState::Standing};
    float     m_vaultTimer{0};
    float     m_leanAngle{0};

    static constexpr float WALK_SPEED    = 3.0f;  ///< m/s
    static constexpr float SPRINT_SPEED  = 6.5f;
    static constexpr float CROUCH_SPEED  = 1.5f;
    static constexpr float PRONE_SPEED   = 0.5f;
    static constexpr float MAX_PITCH     = 1.48f; ///< ~85°
};

} // namespace shooter
