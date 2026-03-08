// ---------------------------------------------------------------------------
// Player.cpp — PlayerController implementation.
// ---------------------------------------------------------------------------

#include "gameplay/Player.h"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace shooter {

bool PlayerController::initialize(ECSWorld& world, Vec3 spawnPosition) {
    m_entityID = world.createEntity();
    m_position = spawnPosition;
    m_eyeHeight = EYE_HEIGHT_STAND;
    m_targetEyeHeight = EYE_HEIGHT_STAND;

    TransformComponent tf;
    tf.x = spawnPosition.x; tf.y = spawnPosition.y; tf.z = spawnPosition.z;
    world.addComponent<TransformComponent>(m_entityID, tf);
    world.addComponent<VelocityComponent>(m_entityID);
    world.addComponent<HealthComponent>(m_entityID, {100.f, 100.f, 0.f});
    world.addComponent<RenderableComponent>(m_entityID);

    std::cout << "[Player] Spawned at ("
              << spawnPosition.x << ", "
              << spawnPosition.y << ", "
              << spawnPosition.z << ")\n";
    return true;
}

void PlayerController::destroy(ECSWorld& world) {
    world.destroyEntity(m_entityID);
    m_entityID = INVALID_ENTITY;
}

void PlayerController::update(ECSWorld& world, f32 dt, const PlayerInput& input) {
    handleLook(dt, input);
    handleStanceTransition(input);
    handleMovement(world, dt, input);
    applyGravity(dt);
    checkVaultOrClimb(world, input);

    // Smooth eye height transition (stance change animation).
    constexpr f32 eyeSpeed = 5.f;
    m_eyeHeight += (m_targetEyeHeight - m_eyeHeight) * eyeSpeed * dt;

    // Sync ECS transform.
    if (auto* tf = world.getComponent<TransformComponent>(m_entityID)) {
        tf->x = m_position.x;
        tf->y = m_position.y;
        tf->z = m_position.z;
        // Store yaw in the quaternion Y component (simplified).
        tf->qy = std::sin(m_yaw * 0.5f);
        tf->qw = std::cos(m_yaw * 0.5f);
    }
    if (auto* vl = world.getComponent<VelocityComponent>(m_entityID)) {
        vl->vx = m_velocity.x;
        vl->vy = m_velocity.y;
        vl->vz = m_velocity.z;
    }
}

// ---------------------------------------------------------------------------
// Look (mouse)
// ---------------------------------------------------------------------------
void PlayerController::handleLook(f32 /*dt*/, const PlayerInput& input) {
    m_yaw   -= input.mouseDeltaX * MOUSE_SENS;
    m_pitch -= input.mouseDeltaY * MOUSE_SENS;

    // Clamp pitch to ±85°.
    constexpr f32 pitchLimit = 1.483f; // ~85°
    m_pitch = std::clamp(m_pitch, -pitchLimit, pitchLimit);

    // Lean roll
    if (input.leanLeft)       m_cameraRoll =  LEAN_ANGLE;
    else if (input.leanRight) m_cameraRoll = -LEAN_ANGLE;
    else                      m_cameraRoll  = 0.f;
}

// ---------------------------------------------------------------------------
// Movement
// ---------------------------------------------------------------------------
void PlayerController::handleMovement(ECSWorld& /*world*/, f32 dt,
                                       const PlayerInput& input) {
    // Resolve desired movement speed from current state.
    f32 speed = WALK_SPEED;
    if      (m_state == PlayerState::Running)   speed = RUN_SPEED;
    else if (m_state == PlayerState::Crouching) speed = CROUCH_SPEED;
    else if (m_state == PlayerState::Prone)     speed = PRONE_SPEED;

    // Build movement vector in world space from yaw.
    f32 sinYaw = std::sin(m_yaw);
    f32 cosYaw = std::cos(m_yaw);

    f32 forwardX =  cosYaw * input.moveForward + sinYaw * input.moveRight;
    f32 forwardZ = -sinYaw * input.moveForward + cosYaw * input.moveRight;

    // Only set XZ velocity (gravity/jump handle Y separately).
    m_velocity.x = forwardX * speed;
    m_velocity.z = forwardZ * speed;

    // Jump.
    if (input.jump && m_isGrounded) {
        m_velocity.y  = JUMP_IMPULSE;
        m_isGrounded  = false;
        m_state       = PlayerState::Walking; // Clear prone/crouch on jump
    }

    m_position.x += m_velocity.x * dt;
    m_position.z += m_velocity.z * dt;
    m_position.y += m_velocity.y * dt;

    // Determine idle vs walking/running from horizontal speed.
    f32 horizSpeed = std::sqrt(m_velocity.x*m_velocity.x +
                               m_velocity.z*m_velocity.z);
    if (horizSpeed < 0.1f && m_state == PlayerState::Walking)
        m_state = PlayerState::Idle;
    else if (horizSpeed > 0.1f && m_state == PlayerState::Idle)
        m_state = PlayerState::Walking;
}

// ---------------------------------------------------------------------------
// Gravity
// ---------------------------------------------------------------------------
void PlayerController::applyGravity(f32 dt) {
    if (!m_isGrounded) {
        m_velocity.y += GRAVITY * dt;
    }
    // Simple ground plane at y = 0 (terrain would replace this).
    if (m_position.y <= 0.f) {
        m_position.y = 0.f;
        m_velocity.y = 0.f;
        m_isGrounded = true;
    }
}

// ---------------------------------------------------------------------------
// Stance transitions
// ---------------------------------------------------------------------------
void PlayerController::handleStanceTransition(const PlayerInput& input) {
    if (input.run && m_isGrounded && m_state != PlayerState::Prone) {
        m_state = PlayerState::Running;
        m_targetEyeHeight = EYE_HEIGHT_STAND;
    } else if (input.prone) {
        m_state = PlayerState::Prone;
        m_targetEyeHeight = EYE_HEIGHT_PRONE;
    } else if (input.crouch) {
        m_state = PlayerState::Crouching;
        m_targetEyeHeight = EYE_HEIGHT_CROUCH;
    } else if (!input.run && m_state == PlayerState::Running) {
        m_state = PlayerState::Walking;
        m_targetEyeHeight = EYE_HEIGHT_STAND;
    }
}

// ---------------------------------------------------------------------------
// Vault / climb detection
// ---------------------------------------------------------------------------
void PlayerController::checkVaultOrClimb(ECSWorld& /*world*/,
                                          const PlayerInput& input) {
    if (!input.interact) return;
    // TODO: cast a sphere ahead of the player; if a low obstacle is found
    // (< 1.2 m) trigger vault; if a ledge is found trigger climb.
    std::cout << "[Player] Interact pressed — vault/climb check (stub)\n";
}

// ---------------------------------------------------------------------------
// Camera position
// ---------------------------------------------------------------------------
Vec3 PlayerController::getCameraPosition() const {
    // Lean offset: shift camera sideways along the camera's right vector.
    f32 leanOffset = m_cameraRoll * 0.4f; // ~40 cm at full lean
    f32 sinYaw = std::sin(m_yaw);
    f32 cosYaw = std::cos(m_yaw);
    return {
        m_position.x + sinYaw * leanOffset,
        m_position.y + m_eyeHeight,
        m_position.z + cosYaw * leanOffset
    };
}

} // namespace shooter
