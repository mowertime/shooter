#include "gameplay/Player.h"
#include "core/Logger.h"
#include <cmath>
#include <algorithm>

namespace shooter {

Player::Player(EntityRegistry& registry, PhysicsWorld& physics)
    : m_registry(registry)
    , m_physics(physics)
{
    m_entity = m_registry.createEntity();
    m_registry.addComponent(m_entity, TransformComponent{});
    m_registry.addComponent(m_entity, HealthComponent{100.0f, 100.0f});
    m_registry.addComponent(m_entity, WeaponComponent{});

    m_physicsBodyId = m_physics.addRigidBody(0.0f, 1.8f, 0.0f, 85.0f);
    LOG_INFO("Player", "Created entity id=%llu",
             static_cast<unsigned long long>(m_entity.id));
}

Player::~Player() {
    m_registry.destroyEntity(m_entity);
    m_physics.removeBody(m_physicsBodyId);
}

void Player::update(const PlayerInput& input, float dt) {
    handleLook(input, dt);
    handleMovement(input, dt);
    handleVault(dt);

    // Sync physics body position back to transform component.
    float px, py, pz;
    if (m_physics.getPosition(m_physicsBodyId, px, py, pz)) {
        auto* t = m_registry.getComponent<TransformComponent>(m_entity);
        if (t) { t->x = px; t->y = py; t->z = pz; t->yaw = m_yaw; t->pitch = m_pitch; }
    }

    // Weapon cooldown tick.
    auto* w = m_registry.getComponent<WeaponComponent>(m_entity);
    if (w) {
        if (w->reloadTimer  > 0) w->reloadTimer  -= dt;
        if (w->fireRateTimer > 0) w->fireRateTimer -= dt;

        if (input.reload && w->reloadTimer <= 0.0f) {
            i32 needed = 30 - w->ammoInMag;
            i32 take   = std::min(needed, w->ammoReserve);
            w->ammoInMag   += take;
            w->ammoReserve -= take;
            w->reloadTimer  = 2.5f;
        }
    }
}

void Player::handleLook(const PlayerInput& input, float /*dt*/) {
    m_yaw   += input.lookYaw;
    m_pitch  = std::clamp(m_pitch + input.lookPitch, -MAX_PITCH, MAX_PITCH);

    float targetLean = 0.0f;
    if (input.leanLeft)  targetLean = -0.35f;
    if (input.leanRight) targetLean =  0.35f;
    m_leanAngle += (targetLean - m_leanAngle) * 0.2f;
}

void Player::handleMovement(const PlayerInput& input, float dt) {
    // Choose speed based on stance.
    float speed = WALK_SPEED;
    if (input.crouch) { m_moveState = MoveState::Crouching; speed = CROUCH_SPEED; }
    else if (input.prone) { m_moveState = MoveState::Prone; speed = PRONE_SPEED; }
    else if (m_moveState != MoveState::Vaulting) {
        m_moveState = MoveState::Standing;
    }

    // Compute world-space movement direction.
    float sinYaw = std::sin(m_yaw);
    float cosYaw = std::cos(m_yaw);

    float fwdX = cosYaw * input.moveForward + sinYaw * input.moveSide;
    float fwdZ = -sinYaw * input.moveForward + cosYaw * input.moveSide;
    float len  = std::sqrt(fwdX * fwdX + fwdZ * fwdZ);
    if (len > 1.0f) { fwdX /= len; fwdZ /= len; }

    float vx = fwdX * speed;
    float vz = fwdZ * speed;

    if (input.jump && m_moveState == MoveState::Standing) {
        m_physics.applyImpulse(m_physicsBodyId, 0.0f, 85.0f * 4.5f, 0.0f);
    }

    // Set horizontal velocity via impulse (override previous horizontal speed).
    m_physics.applyImpulse(m_physicsBodyId, vx * dt, 0.0f, vz * dt);

    if (input.vault && m_moveState != MoveState::Vaulting) {
        m_moveState   = MoveState::Vaulting;
        m_vaultTimer  = 0.6f;
    }
}

void Player::handleVault(float dt) {
    if (m_moveState != MoveState::Vaulting) return;
    m_vaultTimer -= dt;
    if (m_vaultTimer <= 0.0f) {
        m_vaultTimer = 0.0f;
        m_moveState  = MoveState::Standing;
    }
    // Smoothly move over the obstacle (simplified).
    m_physics.applyImpulse(m_physicsBodyId, 0.0f, 85.0f * 2.0f * dt, 0.3f);
}

void Player::setPosition(float x, float y, float z) {
    m_physics.setPosition(m_physicsBodyId, x, y, z);
}

void Player::getPosition(float& x, float& y, float& z) const {
    m_physics.getPosition(m_physicsBodyId, x, y, z);
}

float Player::health() const {
    auto* h = m_registry.getComponent<HealthComponent>(m_entity);
    return h ? h->current : 0.0f;
}

bool Player::isAlive() const {
    auto* h = m_registry.getComponent<HealthComponent>(m_entity);
    return h && h->isAlive();
}

} // namespace shooter
