#pragma once

#include "core/Platform.h"
#include <memory>
#include <vector>

namespace shooter {

/// Contact event between two rigid bodies.
struct ContactEvent {
    u32   bodyA{0};
    u32   bodyB{0};
    float impulse{0.0f};
    float nx{0}, ny{0}, nz{0}; ///< Contact normal
};

/// Physics world configuration.
struct PhysicsWorldDesc {
    float gravityX{0.0f};
    float gravityY{-9.81f};
    float gravityZ{0.0f};
    u32   maxBodies{100'000};
    u32   substeps{4};       ///< Integration sub-steps per frame
    bool  enableCcd{true};   ///< Continuous collision detection
};

/// Central physics simulation.
///
/// Uses a semi-implicit Euler integrator with a broad-phase BVH and
/// narrow-phase GJK/EPA.  Destructible geometry is handled via the
/// DestructionSystem which fractures meshes and spawns debris bodies.
class PhysicsWorld {
public:
    explicit PhysicsWorld(const PhysicsWorldDesc& desc = {});
    ~PhysicsWorld();

    /// Advance the simulation by \p dt seconds.
    void step(float dt);

    /// Add a new rigid body at position (x, y, z) and return its id.
    u32  addRigidBody(float x, float y, float z, float mass);

    /// Remove a rigid body by id.
    void removeBody(u32 bodyId);

    /// Apply an impulse to a body in world space.
    void applyImpulse(u32 bodyId, float ix, float iy, float iz);

    /// Set body position.
    void setPosition(u32 bodyId, float x, float y, float z);

    /// Get body position.
    bool getPosition(u32 bodyId, float& x, float& y, float& z) const;

    /// Shoot a ray and return the id of the first hit body, or UINT32_MAX.
    u32  raycast(float ox, float oy, float oz,
                 float dx, float dy, float dz,
                 float maxDist, float& hitDist) const;

    const std::vector<ContactEvent>& contactEvents() const;
    void clearContactEvents();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace shooter
