#include "world/WorldSystem.h"
#include "world/Terrain.h"
#include "world/WorldStreamer.h"
#include "world/BiomeSystem.h"
#include "world/ProceduralCity.h"
#include "core/Threading.h"
#include "core/Logger.h"

namespace shooter {

// A module-level thread pool for world streaming.
static ThreadPool* s_worldThreadPool = nullptr;

WorldSystem::WorldSystem(const WorldDesc& desc)
    : m_desc(desc)
{
    if (!s_worldThreadPool) {
        s_worldThreadPool = new ThreadPool(2);
    }

    u32 hmRes = 4096; // 4096×4096 heightmap for a 200 km world → ~50 m/sample
    m_terrain = std::make_unique<Terrain>(
        desc.widthKm  * 1000.0f,
        desc.heightKm * 1000.0f,
        hmRes, desc.seed);

    m_streamer = std::make_unique<WorldStreamer>(desc, *s_worldThreadPool);
    m_biomes   = std::make_unique<BiomeSystem>();
    m_cities   = std::make_unique<ProceduralCity>();
}

WorldSystem::~WorldSystem() = default;

void WorldSystem::init() {
    m_biomes->generateBiomes(m_desc.seed,
                              m_desc.widthKm  * 1000.0f,
                              m_desc.heightKm * 1000.0f);
    LOG_INFO("WorldSystem", "World initialised (%.0f x %.0f km)",
             m_desc.widthKm, m_desc.heightKm);
}

void WorldSystem::shutdown() {
    if (s_worldThreadPool) {
        s_worldThreadPool->waitIdle();
        delete s_worldThreadPool;
        s_worldThreadPool = nullptr;
    }
}

void WorldSystem::update(float playerX, float playerZ, float /*dt*/) {
    m_streamer->update(playerX, playerZ);
}

float WorldSystem::sampleHeight(float x, float z) const {
    return m_terrain->sampleHeight(x, z);
}

u8 WorldSystem::sampleBiome(float x, float z) const {
    return static_cast<u8>(m_biomes->sampleBiome(x, z));
}

Terrain&        WorldSystem::terrain()  { return *m_terrain;  }
WorldStreamer&  WorldSystem::streamer() { return *m_streamer; }
BiomeSystem&    WorldSystem::biomes()   { return *m_biomes;   }
ProceduralCity& WorldSystem::cities()   { return *m_cities;   }

} // namespace shooter
