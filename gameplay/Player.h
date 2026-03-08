#pragma once

// ---------------------------------------------------------------------------
// Player.h — First-person player controller.
// ---------------------------------------------------------------------------

#include <string>
#include "engine/platform/Platform.h"
#include "engine/physics/Physics.h"  // Vec3
#include "engine/ecs/ECS.h"

namespace shooter {

// ---------------------------------------------------------------------------
// PlayerState — the current locomotion / stance state.
// ---------------------------------------------------------------------------
enum class PlayerState {
    Idle,
    Walking,
    Running,
    Crouching,
    Prone,
    Vaulting,
    Climbing,
    LeanLeft,
    LeanRight,
    InVehicle,
    Dead,
};

// ---------------------------------------------------------------------------
// PlayerInput — raw inputs from keyboard/gamepad for one frame.
// ---------------------------------------------------------------------------
struct PlayerInput {
    f32  moveForward{0};   ///< [-1, 1]
    f32  moveRight{0};     ///< [-1, 1]
    f32  mouseDeltaX{0};   ///< pixels
    f32  mouseDeltaY{0};
    bool jump{false};
    bool run{false};
    bool crouch{false};
    bool prone{false};
    bool leanLeft{false};
    bool leanRight{false};
    bool fire{false};
    bool aimDownSights{false};
    bool reload{false};
    bool interact{false};  ///< vault / climb / enter vehicle
};

// ---------------------------------------------------------------------------
// PlayerController — handles movement physics, stance transitions, camera.
// ---------------------------------------------------------------------------
class PlayerController {
public:
    // Movement tuning constants.
    static constexpr f32 WALK_SPEED    = 3.0f;  ///< m/s
    static constexpr f32 RUN_SPEED     = 6.5f;
    static constexpr f32 CROUCH_SPEED  = 1.5f;
    static constexpr f32 PRONE_SPEED   = 0.8f;
    static constexpr f32 JUMP_IMPULSE  = 5.0f;
    static constexpr f32 GRAVITY       = -9.81f;
    static constexpr f32 MOUSE_SENS    = 0.002f; ///< radians per pixel
    static constexpr f32 LEAN_ANGLE    = 0.25f;  ///< radians

    bool initialize(ECSWorld& world, Vec3 spawnPosition);
    void update(ECSWorld& world, f32 dt, const PlayerInput& input);
    void destroy(ECSWorld& world);

    // ---- Accessors ---------------------------------------------------------
    [[nodiscard]] EntityID    entityID()    const { return m_entityID; }
    [[nodiscard]] PlayerState state()       const { return m_state; }
    [[nodiscard]] Vec3        position()    const { return m_position; }
    [[nodiscard]] f32         yaw()         const { return m_yaw; }
    [[nodiscard]] f32         pitch()       const { return m_pitch; }
    [[nodiscard]] f32         cameraRoll()  const { return m_cameraRoll; }

    // ---- Camera -----------------------------------------------------------
    /// Returns the camera eye position accounting for stance and lean.
    [[nodiscard]] Vec3 getCameraPosition() const;

private:
    void handleMovement(ECSWorld& world, f32 dt, const PlayerInput& input);
    void handleLook(f32 dt, const PlayerInput& input);
    void handleStanceTransition(const PlayerInput& input);
    void applyGravity(f32 dt);
    void checkVaultOrClimb(ECSWorld& world, const PlayerInput& input);

    EntityID    m_entityID{INVALID_ENTITY};
    PlayerState m_state{PlayerState::Idle};

    Vec3  m_position{};
    Vec3  m_velocity{};
    f32   m_yaw{0.f};
    f32   m_pitch{0.f};
    f32   m_cameraRoll{0.f};  ///< non-zero during lean
    bool  m_isGrounded{true};

    // Stance-dependent eye heights (metres above foot position)
    static constexpr f32 EYE_HEIGHT_STAND  = 1.70f;
    static constexpr f32 EYE_HEIGHT_CROUCH = 1.10f;
    static constexpr f32 EYE_HEIGHT_PRONE  = 0.35f;
    f32 m_eyeHeight{EYE_HEIGHT_STAND};
    f32 m_targetEyeHeight{EYE_HEIGHT_STAND};
};

} // namespace shooter
