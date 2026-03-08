#include "world/Terrain.h"
#include "core/Logger.h"

#include <cmath>
#include <numbers>

namespace shooter {

// ── Deterministic hash ────────────────────────────────────────────────────────
static float hash2(float x, float z, u32 seed) {
    // Wang hash for pseudo-random value from integer coordinates.
    u32 ix = static_cast<u32>(std::abs(x)) ^ seed;
    u32 iz = static_cast<u32>(std::abs(z)) ^ (seed * 2654435761u);
    u32 h  = ix * 2654435761u ^ iz;
    h ^= h >> 16; h *= 0x45d9f3bu;
    h ^= h >> 16; h *= 0x45d9f3bu;
    h ^= h >> 16;
    return static_cast<float>(h & 0xFFFFu) / 65535.0f;
}

/// Value noise in [0, 1].
static float valueNoise(float x, float z, u32 seed) {
    float fx = std::floor(x);
    float fz = std::floor(z);
    float ux  = x - fx;
    float uz  = z - fz;
    // Smooth step.
    ux = ux * ux * (3.0f - 2.0f * ux);
    uz = uz * uz * (3.0f - 2.0f * uz);

    float a = hash2(fx,       fz,       seed);
    float b = hash2(fx + 1.f, fz,       seed);
    float c = hash2(fx,       fz + 1.f, seed);
    float d = hash2(fx + 1.f, fz + 1.f, seed);
    return a + (b - a) * ux + (c - a) * uz + (a - b - c + d) * ux * uz;
}

float Terrain::fractalNoise(float x, float z, u32 seed,
                             u32 octaves,
                             float lacunarity,
                             float persistence) {
    float value     = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue  = 0.0f;
    for (u32 i = 0; i < octaves; ++i) {
        value    += valueNoise(x * frequency, z * frequency, seed + i) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    return value / maxValue;
}

// ── Terrain ───────────────────────────────────────────────────────────────────

Terrain::Terrain(float worldWidthM, float worldHeightM,
                 u32 heightmapRes, u32 seed)
    : m_worldWidthM(worldWidthM)
    , m_worldHeightM(worldHeightM)
    , m_res(heightmapRes)
{
    m_heightmap.resize(static_cast<size_t>(m_res) * m_res, 0.0f);
    generateProcedural(seed);
    LOG_INFO("Terrain", "Generated %ux%u heightmap for %.1f x %.1f km world",
             m_res, m_res, worldWidthM / 1000.0f, worldHeightM / 1000.0f);
}

void Terrain::generateProcedural(u32 seed) {
    float scaleX = 1.0f / static_cast<float>(m_res);
    float scaleZ = 1.0f / static_cast<float>(m_res);
    float maxAlt = 800.0f; // metres

    for (u32 iz = 0; iz < m_res; ++iz) {
        for (u32 ix = 0; ix < m_res; ++ix) {
            float nx = static_cast<float>(ix) * scaleX * 8.0f;
            float nz = static_cast<float>(iz) * scaleZ * 8.0f;
            float h  = fractalNoise(nx, nz, seed);
            m_heightmap[iz * m_res + ix] = h * maxAlt;
        }
    }
}

float Terrain::sampleHeight(float x, float z) const {
    if (m_res == 0) return 0.0f;
    // Map world coords to heightmap space.
    float nx = x / m_worldWidthM  * static_cast<float>(m_res - 1);
    float nz = z / m_worldHeightM * static_cast<float>(m_res - 1);
    if (nx < 0 || nz < 0 || nx >= m_res - 1 || nz >= m_res - 1) return 0.0f;

    u32 ix = static_cast<u32>(nx);
    u32 iz = static_cast<u32>(nz);
    float ux = nx - static_cast<float>(ix);
    float uz = nz - static_cast<float>(iz);

    float h00 = m_heightmap[ iz      * m_res + ix    ];
    float h10 = m_heightmap[ iz      * m_res + ix + 1];
    float h01 = m_heightmap[(iz + 1) * m_res + ix    ];
    float h11 = m_heightmap[(iz + 1) * m_res + ix + 1];

    return h00 * (1 - ux) * (1 - uz)
         + h10 * ux       * (1 - uz)
         + h01 * (1 - ux) * uz
         + h11 * ux       * uz;
}

void Terrain::sampleNormal(float x, float z,
                            float& nx, float& ny, float& nz) const {
    float eps = m_worldWidthM / static_cast<float>(m_res);
    float hL  = sampleHeight(x - eps, z);
    float hR  = sampleHeight(x + eps, z);
    float hD  = sampleHeight(x, z - eps);
    float hU  = sampleHeight(x, z + eps);

    // Central difference gradient → normal.
    float gx = (hR - hL) / (2.0f * eps);
    float gz = (hU - hD) / (2.0f * eps);
    float len = std::sqrt(gx * gx + 1.0f + gz * gz);
    nx = -gx / len;
    ny =  1.0f / len;
    nz = -gz / len;
}

} // namespace shooter
