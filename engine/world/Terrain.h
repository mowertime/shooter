#pragma once

// ---------------------------------------------------------------------------
// Terrain.h — Heightmap-based terrain system for 200+ km² worlds.
//
// The world is divided into square chunks.  Each chunk stores a heightmap
// and can be at different LOD levels (0 = highest detail near camera).
// ---------------------------------------------------------------------------

#include <vector>
#include <array>
#include <cmath>
#include <cstdint>

#include "engine/platform/Platform.h"
#include "engine/physics/Physics.h"   // Vec3

namespace shooter {

static constexpr u32 TERRAIN_CHUNK_VERTS = 65; ///< per side (must be 2^n + 1)
static constexpr u32 TERRAIN_MAX_LOD     = 5;

/// One terrain chunk — the basic streaming unit.
struct TerrainChunk {
    i32  chunkX{0};   ///< chunk grid coordinate
    i32  chunkZ{0};
    u32  lodLevel{0}; ///< 0 = full detail

    /// Heightmap: (TERRAIN_CHUNK_VERTS >> lodLevel + 1)² f32 samples (metres).
    std::vector<f32> heightmap;

    /// World-space bounding box min/max Y.
    f32 minHeight{0.f};
    f32 maxHeight{100.f};

    /// Renderer buffer handle (set after GPU upload).
    u64 gpuBufferHandle{0};

    [[nodiscard]] bool isLoaded() const { return !heightmap.empty(); }
};

// ---------------------------------------------------------------------------
// Terrain — manages the full heightfield for one world.
// ---------------------------------------------------------------------------
class Terrain {
public:
    /// World size (metres per side) and chunk footprint (metres).
    void initialize(f32 worldSizeMetres, f32 chunkSizeMetres);
    void shutdown();

    // ---- Procedural generation --------------------------------------------
    /// Fill chunk heightmap with fractal Brownian Motion (fBm) noise.
    void generateChunk(TerrainChunk& chunk, u32 seed = 42);

    // ---- Queries ----------------------------------------------------------
    /// Sample terrain height (bilinear interpolation) at world XZ position.
    [[nodiscard]] f32 getHeightAt(f32 worldX, f32 worldZ) const;

    /// Sample surface normal (via central differences) at world XZ.
    [[nodiscard]] Vec3 sampleNormal(f32 worldX, f32 worldZ) const;

    /// Convert world position → chunk grid index.
    void worldToChunk(f32 worldX, f32 worldZ, i32& outCX, i32& outCZ) const;

    [[nodiscard]] f32 worldSize()  const { return m_worldSizeMetres; }
    [[nodiscard]] f32 chunkSize()  const { return m_chunkSizeMetres; }
    [[nodiscard]] i32 chunksPerSide() const { return m_chunksPerSide; }

    /// Direct chunk access (returns nullptr if not resident).
    [[nodiscard]] TerrainChunk* getChunk(i32 cx, i32 cz);

    // ---- Biome system (stub) ----------------------------------------------
    enum class Biome { Grassland, Forest, Desert, Arctic, Urban };
    [[nodiscard]] Biome getBiomeAt(f32 worldX, f32 worldZ) const;

private:
    /// Pseudo-random value [0,1] for position — fast hash noise.
    [[nodiscard]] f32 hashNoise(f32 x, f32 z, u32 seed) const;

    /// Fractal Brownian Motion composed of 'octaves' harmonic layers.
    [[nodiscard]] f32 fbm(f32 x, f32 z, u32 seed, u32 octaves = 6) const;

    f32 m_worldSizeMetres{0.f};
    f32 m_chunkSizeMetres{0.f};
    i32 m_chunksPerSide{0};

    // Flat chunk storage: index = cz * m_chunksPerSide + cx.
    std::vector<TerrainChunk> m_chunks;
};

} // namespace shooter
