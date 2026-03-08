#include "world/BiomeSystem.h"
#include <cmath>
#include <limits>

namespace shooter {

static float pseudoRand(u32 seed) {
    seed ^= seed >> 16; seed *= 0x45d9f3bu;
    seed ^= seed >> 16; seed *= 0x45d9f3bu;
    seed ^= seed >> 16;
    return static_cast<float>(seed & 0xFFFFu) / 65535.0f;
}

void BiomeSystem::generateBiomes(u32 seed, float worldWidthM,
                                  float worldHeightM, u32 numBiomes) {
    m_zones.clear();
    m_zones.reserve(numBiomes);
    const BiomeId types[] = {
        BiomeId::Desert, BiomeId::Temperate, BiomeId::Forest,
        BiomeId::Arctic,  BiomeId::Coastal,   BiomeId::Urban,
    };
    constexpr u32 numTypes = 6;
    for (u32 i = 0; i < numBiomes; ++i) {
        BiomeZone z;
        z.centerX    = pseudoRand(seed + i * 7u)     * worldWidthM;
        z.centerZ    = pseudoRand(seed + i * 13u + 1) * worldHeightM;
        z.id         = types[i % numTypes];
        z.radius     = 15000.0f + pseudoRand(seed + i * 3u) * 20000.0f;
        z.treeDensity = pseudoRand(seed + i * 5u + 2);
        m_zones.push_back(z);
    }
}

BiomeId BiomeSystem::sampleBiome(float x, float z) const {
    float   bestDist = std::numeric_limits<float>::max();
    BiomeId best     = BiomeId::Temperate;
    for (const auto& zone : m_zones) {
        float dx = x - zone.centerX;
        float dz = z - zone.centerZ;
        float d  = dx * dx + dz * dz;
        if (d < bestDist) {
            bestDist = d;
            best     = zone.id;
        }
    }
    return best;
}

const BiomeZone* BiomeSystem::getBiomeZone(BiomeId id) const {
    for (const auto& z : m_zones) {
        if (z.id == id) return &z;
    }
    return nullptr;
}

} // namespace shooter
