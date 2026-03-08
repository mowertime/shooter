#pragma once

// ---------------------------------------------------------------------------
// WorldStreamer.h — Asynchronous chunk loading/unloading system.
//
// Maintains a "view radius" around the camera.  Chunks inside the radius are
// streamed in; chunks outside are evicted.  Loading is done on background
// threads so the main thread is never stalled.
// ---------------------------------------------------------------------------

#include <unordered_map>
#include <queue>
#include <mutex>
#include <atomic>
#include <functional>
#include <thread>

#include "engine/platform/Platform.h"
#include "engine/platform/Threading.h"
#include "Terrain.h"

namespace shooter {

// ---- Streaming state -------------------------------------------------------
enum class StreamState {
    Unloaded,   ///< not in memory
    Loading,    ///< background thread is working on it
    Loaded,     ///< GPU-ready
    Unloading,  ///< being evicted from GPU memory
};

struct WorldCell {
    i32         cx{0};
    i32         cz{0};
    StreamState state{StreamState::Unloaded};
    f32         distanceToCamera{0.f}; ///< updated each frame
    u32         lodLevel{0};
};

// ===========================================================================
// WorldStreamer
// ===========================================================================
class WorldStreamer {
public:
    // Callback invoked on the main thread once a chunk is ready to upload.
    using OnChunkLoaded   = std::function<void(TerrainChunk&)>;
    using OnChunkUnloaded = std::function<void(i32 cx, i32 cz)>;

    WorldStreamer();
    ~WorldStreamer();

    void initialize(Terrain* terrain, u32 numWorkers = 2);
    void shutdown();

    /// Called every frame with the camera's world position.
    void update(f32 camX, f32 camZ);

    /// Flush main-thread callbacks (call from game thread).
    void processCompletedLoads();

    void setViewRadius(f32 metres)  { m_viewRadius  = metres; }
    void setUnloadRadius(f32 metres){ m_unloadRadius = metres; }

    void setOnChunkLoaded(OnChunkLoaded cb)   { m_onLoaded   = std::move(cb); }
    void setOnChunkUnloaded(OnChunkUnloaded cb){ m_onUnloaded = std::move(cb); }

    [[nodiscard]] u32 loadedChunkCount() const { return m_loadedCount.load(); }

private:
    struct LoadRequest { i32 cx; i32 cz; u32 lod; };
    struct LoadResult  { TerrainChunk chunk; };

    void workerLoad(LoadRequest req);

    Terrain*        m_terrain{nullptr};
    f32             m_viewRadius{2000.f};    ///< metres (~2 km default)
    f32             m_unloadRadius{3000.f};

    // Chunk state map keyed by encoded (cx, cz).
    std::unordered_map<u64, WorldCell> m_cells;
    std::mutex                         m_cellMutex;

    // Thread pool for async IO/generation.
    std::unique_ptr<ThreadPool>         m_pool;

    // Results queue: background → main thread (lock-free swap pattern).
    std::vector<LoadResult>  m_completedLoads;
    std::mutex               m_completedMutex;

    std::atomic<u32>         m_loadedCount{0};

    OnChunkLoaded    m_onLoaded;
    OnChunkUnloaded  m_onUnloaded;

    static u64 encodeCell(i32 cx, i32 cz) {
        return (static_cast<u64>(static_cast<u32>(cx)) << 32) |
                static_cast<u32>(cz);
    }
};

} // namespace shooter
