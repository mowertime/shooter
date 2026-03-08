// ---------------------------------------------------------------------------
// World.cpp — World, Terrain, and WorldStreamer implementations.
// ---------------------------------------------------------------------------

#include "engine/world/World.h"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace shooter {

// ===========================================================================
// Terrain
// ===========================================================================

void Terrain::initialize(f32 worldSizeMetres, f32 chunkSizeMetres) {
    m_worldSizeMetres = worldSizeMetres;
    m_chunkSizeMetres = chunkSizeMetres;
    m_chunksPerSide   = static_cast<i32>(
        std::ceil(worldSizeMetres / chunkSizeMetres));

    m_chunks.resize(static_cast<size_t>(m_chunksPerSide * m_chunksPerSide));
    for (i32 cz = 0; cz < m_chunksPerSide; ++cz) {
        for (i32 cx = 0; cx < m_chunksPerSide; ++cx) {
            auto& chunk = m_chunks[static_cast<size_t>(cz * m_chunksPerSide + cx)];
            chunk.chunkX = cx;
            chunk.chunkZ = cz;
        }
    }
    std::cout << "[Terrain] Initialized " << m_chunksPerSide << "x"
              << m_chunksPerSide << " chunks ("
              << worldSizeMetres / 1000.f << " km × "
              << worldSizeMetres / 1000.f << " km)\n";
}

void Terrain::shutdown() {
    m_chunks.clear();
}

// ---- Noise helpers ---------------------------------------------------------

f32 Terrain::hashNoise(f32 x, f32 z, u32 seed) const {
    // Integer hash — fast and good enough for prototype terrain.
    u32 ix = static_cast<u32>(static_cast<i32>(std::floor(x)));
    u32 iz = static_cast<u32>(static_cast<i32>(std::floor(z)));
    u32 h  = ix * 1664525u + iz * 1013904223u + seed * 22695477u;
    h = (h ^ (h >> 16)) * 0x45d9f3bu;
    h = (h ^ (h >> 16));
    return static_cast<f32>(h) / static_cast<f32>(0xFFFFFFFFu);
}

f32 Terrain::fbm(f32 x, f32 z, u32 seed, u32 octaves) const {
    f32 value     = 0.f;
    f32 amplitude = 0.5f;
    f32 frequency = 1.f;
    f32 maxVal    = 0.f;

    for (u32 o = 0; o < octaves; ++o) {
        // Bilinear interpolation between four lattice corners.
        f32 fx = x * frequency;
        f32 fz = z * frequency;
        f32 ix0 = std::floor(fx);
        f32 iz0 = std::floor(fz);
        f32 tx  = fx - ix0;
        f32 tz  = fz - iz0;

        // Smoothstep
        f32 ux = tx * tx * (3.f - 2.f * tx);
        f32 uz = tz * tz * (3.f - 2.f * tz);

        f32 n00 = hashNoise(ix0,       iz0,       seed + o);
        f32 n10 = hashNoise(ix0 + 1.f, iz0,       seed + o);
        f32 n01 = hashNoise(ix0,       iz0 + 1.f, seed + o);
        f32 n11 = hashNoise(ix0 + 1.f, iz0 + 1.f, seed + o);

        f32 n = n00 * (1.f - ux) * (1.f - uz)
              + n10 * ux         * (1.f - uz)
              + n01 * (1.f - ux) * uz
              + n11 * ux         * uz;

        value    += amplitude * n;
        maxVal   += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.f;
    }
    return value / maxVal; // normalised [0,1]
}

void Terrain::generateChunk(TerrainChunk& chunk, u32 seed) {
    u32  verts = TERRAIN_CHUNK_VERTS >> chunk.lodLevel;
    if (verts < 2) verts = 2;
    chunk.heightmap.resize(verts * verts);

    f32 scale = m_chunkSizeMetres / static_cast<f32>(verts - 1);
    f32 noiseScale = 1.f / 512.f; // one noise cycle per 512 m

    chunk.minHeight = 1e9f;
    chunk.maxHeight = -1e9f;

    for (u32 iz = 0; iz < verts; ++iz) {
        for (u32 ix = 0; ix < verts; ++ix) {
            f32 wx = (chunk.chunkX * m_chunkSizeMetres + ix * scale) * noiseScale;
            f32 wz = (chunk.chunkZ * m_chunkSizeMetres + iz * scale) * noiseScale;
            f32 h  = fbm(wx, wz, seed) * 250.f; // height range [0, 250] m
            chunk.heightmap[iz * verts + ix] = h;
            chunk.minHeight = std::min(chunk.minHeight, h);
            chunk.maxHeight = std::max(chunk.maxHeight, h);
        }
    }
}

TerrainChunk* Terrain::getChunk(i32 cx, i32 cz) {
    if (cx < 0 || cx >= m_chunksPerSide || cz < 0 || cz >= m_chunksPerSide)
        return nullptr;
    return &m_chunks[static_cast<size_t>(cz * m_chunksPerSide + cx)];
}

f32 Terrain::getHeightAt(f32 worldX, f32 worldZ) const {
    // Find which chunk and local UV this maps to.
    i32 cx = static_cast<i32>(std::floor(worldX / m_chunkSizeMetres));
    i32 cz = static_cast<i32>(std::floor(worldZ / m_chunkSizeMetres));
    if (cx < 0 || cx >= m_chunksPerSide || cz < 0 || cz >= m_chunksPerSide)
        return 0.f;

    const auto& chunk = m_chunks[static_cast<size_t>(cz * m_chunksPerSide + cx)];
    if (!chunk.isLoaded()) return 0.f;

    u32 verts = TERRAIN_CHUNK_VERTS >> chunk.lodLevel;
    if (verts < 2) return chunk.heightmap.empty() ? 0.f : chunk.heightmap[0];

    f32 localX = std::fmod(worldX, m_chunkSizeMetres);
    f32 localZ = std::fmod(worldZ, m_chunkSizeMetres);
    f32 cellSize = m_chunkSizeMetres / static_cast<f32>(verts - 1);

    i32 ix = static_cast<i32>(localX / cellSize);
    i32 iz = static_cast<i32>(localZ / cellSize);
    ix = std::clamp(ix, 0, static_cast<i32>(verts) - 2);
    iz = std::clamp(iz, 0, static_cast<i32>(verts) - 2);

    f32 tx = (localX - ix * cellSize) / cellSize;
    f32 tz = (localZ - iz * cellSize) / cellSize;

    // Bilinear sample
    f32 h00 = chunk.heightmap[static_cast<size_t>( iz      * verts + ix    )];
    f32 h10 = chunk.heightmap[static_cast<size_t>( iz      * verts + ix + 1)];
    f32 h01 = chunk.heightmap[static_cast<size_t>((iz + 1) * verts + ix    )];
    f32 h11 = chunk.heightmap[static_cast<size_t>((iz + 1) * verts + ix + 1)];

    return h00*(1.f-tx)*(1.f-tz) + h10*tx*(1.f-tz)
         + h01*(1.f-tx)*tz       + h11*tx*tz;
}

Vec3 Terrain::sampleNormal(f32 worldX, f32 worldZ) const {
    // Central differences; step = 1 m
    const f32 eps = 1.f;
    f32 hL = getHeightAt(worldX - eps, worldZ);
    f32 hR = getHeightAt(worldX + eps, worldZ);
    f32 hD = getHeightAt(worldX, worldZ - eps);
    f32 hU = getHeightAt(worldX, worldZ + eps);
    Vec3 n = { hL - hR, 2.f * eps, hD - hU };
    return n.normalised();
}

void Terrain::worldToChunk(f32 worldX, f32 worldZ, i32& outCX, i32& outCZ) const {
    outCX = static_cast<i32>(std::floor(worldX / m_chunkSizeMetres));
    outCZ = static_cast<i32>(std::floor(worldZ / m_chunkSizeMetres));
}

Terrain::Biome Terrain::getBiomeAt(f32 worldX, f32 worldZ) const {
    // Stub: use a very simple moisture/temperature proxy from noise.
    f32 moisture = fbm(worldX * 0.00005f, worldZ * 0.00005f, 9999u, 4);
    if (moisture < 0.2f) return Biome::Desert;
    if (moisture < 0.4f) return Biome::Grassland;
    if (moisture < 0.6f) return Biome::Forest;
    if (moisture < 0.8f) return Biome::Arctic;
    return Biome::Urban;
}

// ===========================================================================
// WorldStreamer
// ===========================================================================

WorldStreamer::WorldStreamer()  = default;
WorldStreamer::~WorldStreamer() { shutdown(); }

void WorldStreamer::initialize(Terrain* terrain, u32 numWorkers) {
    m_terrain = terrain;
    m_pool    = std::make_unique<ThreadPool>(numWorkers);
    std::cout << "[WorldStreamer] Initialized ("
              << numWorkers << " loader threads)\n";
}

void WorldStreamer::shutdown() {
    m_pool.reset();
    std::scoped_lock lock(m_cellMutex);
    m_cells.clear();
    std::cout << "[WorldStreamer] Shutdown\n";
}

void WorldStreamer::update(f32 camX, f32 camZ) {
    if (!m_terrain) return;

    i32 camCX, camCZ;
    m_terrain->worldToChunk(camX, camZ, camCX, camCZ);

    i32 chunkRadius = static_cast<i32>(
        std::ceil(m_viewRadius / m_terrain->chunkSize()));

    std::scoped_lock lock(m_cellMutex);

    // Request loading for visible chunks.
    for (i32 dz = -chunkRadius; dz <= chunkRadius; ++dz) {
        for (i32 dx = -chunkRadius; dx <= chunkRadius; ++dx) {
            i32 cx = camCX + dx;
            i32 cz = camCZ + dz;
            if (cx < 0 || cx >= m_terrain->chunksPerSide()) continue;
            if (cz < 0 || cz >= m_terrain->chunksPerSide()) continue;

            f32 distX = static_cast<f32>(dx) * m_terrain->chunkSize();
            f32 distZ = static_cast<f32>(dz) * m_terrain->chunkSize();
            f32 dist  = std::sqrt(distX*distX + distZ*distZ);
            if (dist > m_viewRadius) continue;

            // LOD: 0 = nearest ring, increases with distance.
            u32 lod = std::min(
                static_cast<u32>(dist / (m_viewRadius / TERRAIN_MAX_LOD)),
                TERRAIN_MAX_LOD - 1);

            u64 key = encodeCell(cx, cz);
            auto it = m_cells.find(key);
            if (it == m_cells.end()) {
                // New cell — queue a load request.
                WorldCell cell;
                cell.cx = cx; cell.cz = cz;
                cell.distanceToCamera = dist;
                cell.state    = StreamState::Loading;
                cell.lodLevel = lod;
                m_cells[key]  = cell;

                LoadRequest req{cx, cz, lod};
                m_pool->submit([this, req]{ workerLoad(req); });
            }
        }
    }

    // Evict distant chunks.
    for (auto it = m_cells.begin(); it != m_cells.end(); ) {
        WorldCell& cell = it->second;
        f32 dx = static_cast<f32>(cell.cx - camCX) * m_terrain->chunkSize();
        f32 dz = static_cast<f32>(cell.cz - camCZ) * m_terrain->chunkSize();
        f32 dist = std::sqrt(dx*dx + dz*dz);
        if (dist > m_unloadRadius && cell.state == StreamState::Loaded) {
            if (m_onUnloaded) m_onUnloaded(cell.cx, cell.cz);
            m_loadedCount.fetch_sub(1, std::memory_order_relaxed);
            it = m_cells.erase(it);
        } else {
            ++it;
        }
    }
}

void WorldStreamer::workerLoad(LoadRequest req) {
    TerrainChunk chunk;
    chunk.chunkX   = req.cx;
    chunk.chunkZ   = req.cz;
    chunk.lodLevel = req.lod;
    // Derive a per-chunk seed from its coordinates so neighbouring chunks
    // have different but deterministic noise patterns.
    u32 chunkSeed = static_cast<u32>(req.cx * 73856093) ^
                    static_cast<u32>(req.cz * 19349663);
    m_terrain->generateChunk(chunk, chunkSeed);

    {
        std::scoped_lock lock(m_completedMutex);
        m_completedLoads.push_back({std::move(chunk)});
    }
}

void WorldStreamer::processCompletedLoads() {
    std::vector<LoadResult> done;
    {
        std::scoped_lock lock(m_completedMutex);
        std::swap(done, m_completedLoads);
    }

    for (auto& result : done) {
        u64 key = encodeCell(result.chunk.chunkX, result.chunk.chunkZ);
        {
            std::scoped_lock lock(m_cellMutex);
            auto it = m_cells.find(key);
            if (it != m_cells.end()) {
                it->second.state = StreamState::Loaded;
            }
        }
        if (m_onLoaded) m_onLoaded(result.chunk);
        m_loadedCount.fetch_add(1, std::memory_order_relaxed);
    }
}

// ===========================================================================
// World
// ===========================================================================

bool World::initialize(const WorldDesc& desc) {
    m_desc = desc;
    // Convert km² to metres-per-side: area = side² → side = sqrt(area) * 1000
    m_sizeMetres = std::sqrt(desc.sizeKm2) * 1000.f;

    m_terrain.initialize(m_sizeMetres, desc.chunkSizeMetres);
    m_streamer.initialize(&m_terrain, 2);

    std::cout << "[World] \"" << desc.name << "\" initialized ("
              << desc.sizeKm2 << " km², "
              << m_sizeMetres / 1000.f << " × "
              << m_sizeMetres / 1000.f << " km)\n";
    return true;
}

void World::shutdown() {
    m_streamer.shutdown();
    m_terrain.shutdown();
    std::cout << "[World] Shutdown\n";
}

void World::update(f32 /*dt*/, f32 camX, f32 camZ) {
    m_streamer.update(camX, camZ);
    m_streamer.processCompletedLoads();
}

} // namespace shooter
