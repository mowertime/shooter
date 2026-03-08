#pragma once

#include "core/Platform.h"
#include <vector>
#include <cmath>

namespace shooter {

/// Terrain biome identifiers.
enum class BiomeId : u8 {
    Desert   = 0,
    Temperate = 1,
    Forest   = 2,
    Arctic   = 3,
    Coastal  = 4,
    Urban    = 5,
};

/// Heightmap-based terrain.
///
/// The terrain is subdivided into tiles; each tile has a heightmap
/// stored as 16-bit integers.  The tessellation shader reads the
/// raw heightmap and LODs are derived on the GPU.
class Terrain {
public:
    /// Create a terrain of the given world-space dimensions (metres).
    Terrain(float worldWidthM, float worldHeightM,
            u32 heightmapRes, u32 seed);
    ~Terrain() = default;

    void generateProcedural(u32 seed);

    /// Returns the terrain height in metres at world position (x, z).
    /// Uses bilinear interpolation between heightmap samples.
    float sampleHeight(float x, float z) const;

    /// Returns terrain surface normal at (x, z).
    void  sampleNormal(float x, float z, float& nx, float& ny, float& nz) const;

    float worldWidthM()  const { return m_worldWidthM;  }
    float worldHeightM() const { return m_worldHeightM; }
    u32   heightmapRes() const { return m_res;          }

private:
    float              m_worldWidthM;
    float              m_worldHeightM;
    u32                m_res;         ///< Heightmap resolution (samples per axis)
    std::vector<float> m_heightmap;  ///< Flat row-major array [z * res + x]

    /// Simple fractal noise for procedural generation.
    static float fractalNoise(float x, float z, u32 seed,
                               u32 octaves = 8,
                               float lacunarity = 2.0f,
                               float persistence = 0.5f);
};

} // namespace shooter
