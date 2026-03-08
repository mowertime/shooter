#include "gameplay/Vehicle.h"

namespace shooter {

VehicleInstance::VehicleInstance(EntityRegistry& registry,
                                  PhysicsWorld& physicsWorld,
                                  VehiclePhysics& vehiclePhysics,
                                  AircraftPhysics& aircraftPhysics,
                                  const VehicleDesc& desc,
                                  float x, float y, float z)
    : m_registry(registry)
    , m_physics(physicsWorld)
    , m_vehiclePhysics(vehiclePhysics)
    , m_aircraftPhysics(aircraftPhysics)
    , m_desc(desc)
{
    m_entity = m_registry.createEntity();
    m_registry.addComponent(m_entity, TransformComponent{x, y, z});
    m_registry.addComponent(m_entity, HealthComponent{desc.maxHealth, desc.maxHealth});
    VehicleComponent vc;
    vc.vehicleTypeId = desc.meshAssetId;
    m_registry.addComponent(m_entity, vc);

    m_isAircraft = (desc.category == VehicleCategory::Helicopter ||
                    desc.category == VehicleCategory::FixedWingJet);

    if (!m_isAircraft) {
        VehicleParams vp;
        vp.mass = (desc.category == VehicleCategory::Tank) ? 60000.0f : 8000.0f;
        m_vehicleId = vehiclePhysics.createVehicle(physicsWorld, vp, x, y, z);
    } else {
        AircraftParams ap;
        ap.isRotary = (desc.category == VehicleCategory::Helicopter);
        m_vehicleId = aircraftPhysics.createAircraft(physicsWorld, ap, x, y, z);
    }
}

VehicleInstance::~VehicleInstance() {
    m_registry.destroyEntity(m_entity);
    if (!m_isAircraft)
        m_vehiclePhysics.destroyVehicle(m_physics, m_vehicleId);
    else
        m_aircraftPhysics.destroyAircraft(m_physics, m_vehicleId);
}

void VehicleInstance::update(float /*dt*/) {
    // Sync physics state back to the ECS VehicleComponent.
    float px, py, pz;
    if (m_physics.getPosition(
            m_isAircraft
                ? m_aircraftPhysics.bodyId(m_vehicleId)
                : m_vehiclePhysics.bodyId(m_vehicleId),
            px, py, pz)) {
        auto* t = m_registry.getComponent<TransformComponent>(m_entity);
        if (t) { t->x = px; t->y = py; t->z = pz; }
    }
}

void VehicleInstance::setInput(float throttle, float steer, float brake) {
    if (!m_isAircraft)
        m_vehiclePhysics.setControls(m_vehicleId, throttle, steer, brake);
    else
        m_aircraftPhysics.setControls(m_vehicleId, steer, 0.0f, 0.0f, throttle);
}

void VehicleInstance::applyDamage(float dmg) {
    auto* h = m_registry.getComponent<HealthComponent>(m_entity);
    if (!h) return;
    h->current = std::max(0.0f, h->current - dmg);
    auto* vc   = m_registry.getComponent<VehicleComponent>(m_entity);
    if (vc && h->current <= 0.0f) vc->isDestroyed = true;
}

bool  VehicleInstance::isAlive() const {
    auto* h = m_registry.getComponent<HealthComponent>(m_entity);
    return h && h->isAlive();
}

float VehicleInstance::health() const {
    auto* h = m_registry.getComponent<HealthComponent>(m_entity);
    return h ? h->current : 0.0f;
}

} // namespace shooter
