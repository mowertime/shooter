#pragma once

#include "ecs/EntityRegistry.h"
#include <vector>
#include <unordered_map>

namespace shooter {

/// Snapshot of a replicated entity's state at a given tick.
struct EntitySnapshot {
    u64   entityId{Entity::INVALID};
    u32   tickNumber{0};
    float px{0}, py{0}, pz{0};
    float vx{0}, vy{0}, vz{0};
    float yaw{0};
    u8    health{100};
};

/// Manages entity state replication between server and clients.
///
/// The server serialises component changes into snapshots; clients
/// interpolate between received snapshots for smooth rendering.
class EntityReplication {
public:
    /// Server: collect dirty entity states and build a snapshot buffer.
    void gatherSnapshots(const EntityRegistry& registry,
                         u32 tick,
                         std::vector<EntitySnapshot>& outSnapshots);

    /// Client: apply a received snapshot, resolving conflicts with
    /// the local predicted state.
    void applySnapshot(EntityRegistry& registry,
                       const EntitySnapshot& snap);

    /// Client: reconcile local prediction with the authoritative server state.
    void reconcile(EntityRegistry& registry,
                   const std::vector<EntitySnapshot>& serverSnaps,
                   u32 acknowledgedTick);

private:
    std::unordered_map<u64, EntitySnapshot> m_lastKnown;
};

} // namespace shooter
