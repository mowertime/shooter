#include "network/EntityReplication.h"
#include "core/Logger.h"

namespace shooter {

void EntityReplication::gatherSnapshots(
        const EntityRegistry& /*registry*/,
        u32 tick,
        std::vector<EntitySnapshot>& outSnapshots) {
    (void)tick;
    // In a full implementation: iterate entities with replicated components
    // and produce an EntitySnapshot for each that has changed since the
    // last acknowledged tick.
    outSnapshots.clear();
}

void EntityReplication::applySnapshot(EntityRegistry& registry,
                                       const EntitySnapshot& snap) {
    Entity e;
    e.id = snap.entityId;
    if (!registry.isAlive(e)) return;

    auto* t = registry.getComponent<TransformComponent>(e);
    if (t) { t->x = snap.px; t->y = snap.py; t->z = snap.pz; t->yaw = snap.yaw; }
}

void EntityReplication::reconcile(
        EntityRegistry& /*registry*/,
        const std::vector<EntitySnapshot>& /*serverSnaps*/,
        u32 /*acknowledgedTick*/) {
    // Rewind local state to acknowledgedTick, re-apply server positions,
    // then re-simulate inputs up to the current tick.
    LOG_DEBUG("EntityReplication", "Reconcile (stub)");
}

} // namespace shooter
