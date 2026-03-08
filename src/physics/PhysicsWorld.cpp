#include "physics/PhysicsWorld.h"
#include "core/Logger.h"

#include <cmath>
#include <limits>
#include <algorithm>

namespace shooter {

struct Body {
    float px{0}, py{0}, pz{0};     ///< Position
    float vx{0}, vy{0}, vz{0};     ///< Velocity
    float mass{1.0f};
    bool  active{true};
};

struct PhysicsWorld::Impl {
    PhysicsWorldDesc         desc;
    std::vector<Body>        bodies;
    std::vector<u32>         freeList;
    std::vector<ContactEvent> contacts;
};

PhysicsWorld::PhysicsWorld(const PhysicsWorldDesc& desc)
    : m_impl(std::make_unique<Impl>()) {
    m_impl->desc = desc;
    m_impl->bodies.reserve(desc.maxBodies);
    LOG_INFO("PhysicsWorld", "Initialised (gravity=%.2f, substeps=%u)",
             desc.gravityY, desc.substeps);
}

PhysicsWorld::~PhysicsWorld() = default;

void PhysicsWorld::step(float dt) {
    float subDt = dt / static_cast<float>(m_impl->desc.substeps);
    for (u32 s = 0; s < m_impl->desc.substeps; ++s) {
        for (auto& b : m_impl->bodies) {
            if (!b.active || b.mass <= 0.0f) continue;

            // Apply gravity.
            b.vx += m_impl->desc.gravityX * subDt;
            b.vy += m_impl->desc.gravityY * subDt;
            b.vz += m_impl->desc.gravityZ * subDt;

            // Integrate position.
            b.px += b.vx * subDt;
            b.py += b.vy * subDt;
            b.pz += b.vz * subDt;

            // Simple ground plane at y = 0.
            if (b.py < 0.0f) {
                b.py  = 0.0f;
                b.vy  = -b.vy * 0.5f; // restitution
            }
        }
    }
}

u32 PhysicsWorld::addRigidBody(float x, float y, float z, float mass) {
    u32 id;
    if (!m_impl->freeList.empty()) {
        id = m_impl->freeList.back();
        m_impl->freeList.pop_back();
        m_impl->bodies[id] = {x, y, z, 0, 0, 0, mass, true};
    } else {
        id = static_cast<u32>(m_impl->bodies.size());
        m_impl->bodies.push_back({x, y, z, 0, 0, 0, mass, true});
    }
    return id;
}

void PhysicsWorld::removeBody(u32 id) {
    if (id >= m_impl->bodies.size()) return;
    m_impl->bodies[id].active = false;
    m_impl->freeList.push_back(id);
}

void PhysicsWorld::applyImpulse(u32 id, float ix, float iy, float iz) {
    if (id >= m_impl->bodies.size()) return;
    Body& b = m_impl->bodies[id];
    if (b.mass <= 0.0f) return;
    float inv = 1.0f / b.mass;
    b.vx += ix * inv;
    b.vy += iy * inv;
    b.vz += iz * inv;
}

void PhysicsWorld::setPosition(u32 id, float x, float y, float z) {
    if (id < m_impl->bodies.size()) {
        m_impl->bodies[id].px = x;
        m_impl->bodies[id].py = y;
        m_impl->bodies[id].pz = z;
    }
}

bool PhysicsWorld::getPosition(u32 id, float& x, float& y, float& z) const {
    if (id >= m_impl->bodies.size()) return false;
    const Body& b = m_impl->bodies[id];
    x = b.px; y = b.py; z = b.pz;
    return b.active;
}

u32 PhysicsWorld::raycast(float ox, float oy, float oz,
                           float dx, float dy, float dz,
                           float maxDist, float& hitDist) const {
    // Simplified sphere-casting against body positions with unit radius.
    float bestDist = maxDist;
    u32   bestId   = UINT32_MAX;
    for (u32 i = 0; i < m_impl->bodies.size(); ++i) {
        const Body& b = m_impl->bodies[i];
        if (!b.active) continue;
        float ex = b.px - ox, ey = b.py - oy, ez = b.pz - oz;
        float t  = ex * dx + ey * dy + ez * dz;
        if (t < 0.0f || t > bestDist) continue;
        float cx = ox + dx * t - b.px;
        float cy = oy + dy * t - b.py;
        float cz = oz + dz * t - b.pz;
        float d2 = cx*cx + cy*cy + cz*cz;
        if (d2 < 1.0f && t < bestDist) {
            bestDist = t;
            bestId   = i;
        }
    }
    hitDist = bestDist;
    return bestId;
}

const std::vector<ContactEvent>& PhysicsWorld::contactEvents() const {
    return m_impl->contacts;
}

void PhysicsWorld::clearContactEvents() {
    m_impl->contacts.clear();
}

} // namespace shooter
