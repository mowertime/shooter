#pragma once

#include "world/WorldSystem.h"
#include "core/Threading.h"
#include <unordered_map>
#include <vector>
#include <mutex>

namespace shooter {

/// Manages asynchronous loading and unloading of world tiles.
///
/// The streamer maintains a set of "active" tiles around the player.
/// Tiles beyond the unload radius are evicted; tiles within the load
/// radius that are not yet loaded are queued for async I/O.
class WorldStreamer {
public:
    explicit WorldStreamer(const WorldDesc& desc, ThreadPool& pool);

    /// Advance streaming around \p (playerX, playerZ).
    void update(float playerX, float playerZ);

    u32 loadedTileCount() const;
    u32 pendingTileCount() const;

private:
    void requestLoad  (i32 tx, i32 tz);
    void requestUnload(i32 tx, i32 tz);
    u64  tileKey(i32 tx, i32 tz) const;

    const WorldDesc&            m_desc;
    ThreadPool&                 m_pool;
    std::unordered_map<u64, WorldTile> m_tiles;
    mutable std::mutex          m_tilesMutex;
    float                       m_tileSize{1000.0f}; ///< Metres per tile
};

} // namespace shooter
