#pragma once

// ---------------------------------------------------------------------------
// World.h — Top-level world system composing Terrain + WorldStreamer.
// ---------------------------------------------------------------------------

#include <memory>
#include <string>

#include "engine/platform/Platform.h"
#include "Terrain.h"
#include "WorldStreamer.h"

namespace shooter {

struct WorldDesc {
    f32         sizeKm2{200.f};         ///< total area in km² (200 = 14.1 × 14.1 km)
    f32         chunkSizeMetres{512.f};  ///< each chunk covers 512 m per side
    u32         seed{12345};
    std::string name{"DefaultWorld"};
};

// ===========================================================================
// World — owns terrain, streaming, and biome/weather stubs.
// ===========================================================================
class World {
public:
    World()  = default;
    ~World() = default;

    bool initialize(const WorldDesc& desc);
    void shutdown();

    /// Called every frame.  camX/camZ = camera world position.
    void update(f32 dt, f32 camX, f32 camZ);

    // ---- Accessors --------------------------------------------------------
    [[nodiscard]] Terrain*       getTerrain()       { return &m_terrain; }
    [[nodiscard]] WorldStreamer* getStreamer()       { return &m_streamer; }
    [[nodiscard]] f32            sizeMetres()  const { return m_sizeMetres; }

    // ---- Convenience -------------------------------------------------------
    [[nodiscard]] f32 getHeightAt(f32 x, f32 z) const {
        return const_cast<Terrain&>(m_terrain).getHeightAt(x, z);
    }

private:
    WorldDesc    m_desc;
    f32          m_sizeMetres{0.f};
    Terrain      m_terrain;
    WorldStreamer m_streamer;
};

} // namespace shooter
