#pragma once

#include "world/Terrain.h"
#include <vector>

namespace shooter {

struct BiomeZone {
    float  centerX{0}, centerZ{0};
    float  radius{10000.0f};
    BiomeId id{BiomeId::Temperate};
    float  temperature{20.0f};  ///< °C
    float  humidity{0.5f};      ///< 0..1
    float  treeDensity{0.3f};
};

/// Controls biome distribution across the world using weighted Voronoi.
class BiomeSystem {
public:
    void generateBiomes(u32 seed, float worldWidthM, float worldHeightM,
                        u32 numBiomes = 12);

    BiomeId sampleBiome(float x, float z) const;

    const BiomeZone* getBiomeZone(BiomeId id) const;

    u32 biomeCount() const { return static_cast<u32>(m_zones.size()); }

private:
    std::vector<BiomeZone> m_zones;
};

} // namespace shooter
