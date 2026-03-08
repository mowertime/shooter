#pragma once

#include "core/Platform.h"
#include <vector>

namespace shooter {

/// A single destructible building/structure chunk.
struct DestructibleChunk {
    u32   physicsBodyId{UINT32_MAX};
    u32   meshAssetId{UINT32_MAX};
    float health{100.0f};
    bool  isDestroyed{false};
};

/// Destruction system.
///
/// When a chunk's health reaches zero it fractures into Voronoi debris
/// (pre-computed offline) and spawns debris rigid bodies.
class DestructionSystem {
public:
    /// Register a structure with pre-fractured chunk data.
    u32 addStructure(std::vector<DestructibleChunk> chunks);

    /// Apply damage to a chunk.  Returns true if it was destroyed this call.
    bool applyDamage(u32 structureId, u32 chunkId, float damage);

    /// Tick debris physics lifetime (chunks auto-remove after expiry).
    void update(float dt);

    u32 structureCount() const;

private:
    struct Structure {
        std::vector<DestructibleChunk> chunks;
        float debrisTimer{0};
    };
    std::vector<Structure> m_structures;
};

} // namespace shooter
