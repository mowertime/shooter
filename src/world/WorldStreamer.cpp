#include "world/WorldStreamer.h"
#include "core/Logger.h"
#include <cmath>

namespace shooter {

WorldStreamer::WorldStreamer(const WorldDesc& desc, ThreadPool& pool)
    : m_desc(desc), m_pool(pool) {}

u64 WorldStreamer::tileKey(i32 tx, i32 tz) const {
    return (static_cast<u64>(static_cast<u32>(tx)) << 32) |
            static_cast<u64>(static_cast<u32>(tz));
}

void WorldStreamer::update(float playerX, float playerZ) {
    i32 cx = static_cast<i32>(playerX / m_tileSize);
    i32 cz = static_cast<i32>(playerZ / m_tileSize);
    i32 rad = static_cast<i32>(std::ceil(
        static_cast<float>(m_desc.streamingRadiusM) / m_tileSize));

    // Request loads for tiles within radius.
    for (i32 dz = -rad; dz <= rad; ++dz) {
        for (i32 dx = -rad; dx <= rad; ++dx) {
            if (dx * dx + dz * dz > rad * rad) continue;
            i32 tx = cx + dx;
            i32 tz = cz + dz;
            std::lock_guard lock(m_tilesMutex);
            u64 key = tileKey(tx, tz);
            if (m_tiles.find(key) == m_tiles.end()) {
                requestLoad(tx, tz);
            }
        }
    }

    // Unload tiles beyond 1.5× radius.
    float unloadRadius = static_cast<float>(m_desc.streamingRadiusM) * 1.5f;
    std::lock_guard lock(m_tilesMutex);
    for (auto it = m_tiles.begin(); it != m_tiles.end(); ) {
        const WorldTile& t = it->second;
        float dx = static_cast<float>(t.tileX - cx) * m_tileSize;
        float dz = static_cast<float>(t.tileZ - cz) * m_tileSize;
        float d  = std::sqrt(dx * dx + dz * dz);
        if (d > unloadRadius && t.state == TileState::Loaded) {
            requestUnload(t.tileX, t.tileZ);
            it = m_tiles.erase(it);
        } else {
            ++it;
        }
    }
}

void WorldStreamer::requestLoad(i32 tx, i32 tz) {
    WorldTile tile;
    tile.tileX = tx;
    tile.tileZ = tz;
    tile.state = TileState::Queued;
    u64 key    = tileKey(tx, tz);
    m_tiles[key] = tile;

    // Post async load job.
    m_pool.submit([this, tx, tz, key] {
        std::lock_guard lock(m_tilesMutex);
        auto it = m_tiles.find(key);
        if (it == m_tiles.end()) return;
        it->second.state = TileState::Loaded;
        LOG_DEBUG("WorldStreamer", "Loaded tile (%d, %d)", tx, tz);
    }, JobPriority::High);
}

void WorldStreamer::requestUnload(i32 tx, i32 tz) {
    LOG_DEBUG("WorldStreamer", "Unloaded tile (%d, %d)", tx, tz);
}

u32 WorldStreamer::loadedTileCount() const {
    std::lock_guard lock(m_tilesMutex);
    u32 n = 0;
    for (const auto& [key, t] : m_tiles)
        n += (t.state == TileState::Loaded) ? 1 : 0;
    return n;
}

u32 WorldStreamer::pendingTileCount() const {
    std::lock_guard lock(m_tilesMutex);
    u32 n = 0;
    for (const auto& [key, t] : m_tiles)
        n += (t.state == TileState::Queued || t.state == TileState::Loading) ? 1 : 0;
    return n;
}

} // namespace shooter
