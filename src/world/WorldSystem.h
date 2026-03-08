#pragma once

#include "core/Platform.h"
#include <memory>
#include <string>

namespace shooter {

/// World configuration for a 200+ km² open world.
struct WorldDesc {
    float widthKm{200.0f};        ///< Total world width in kilometres
    float heightKm{200.0f};
    u32   terrainLodLevels{8};
    u32   streamingRadiusM{3000};  ///< Distance at which tiles are loaded
    u32   seed{42};               ///< Procedural generation seed
    std::string worldDataPath;    ///< Path to pre-baked world data (optional)
};

/// World tile state.
enum class TileState : u8 {
    Unloaded,
    Queued,    ///< I/O request submitted
    Loading,
    Loaded,
    Unloading,
};

/// A 1 km × 1 km world tile.
struct WorldTile {
    i32       tileX{0}, tileZ{0};   ///< Grid coordinates
    TileState state{TileState::Unloaded};
    u32       terrainBodyId{UINT32_MAX};
    u32       meshAssetId{UINT32_MAX};
    float     heightmapMin{0};
    float     heightmapMax{500};
};

class Terrain;
class WorldStreamer;
class BiomeSystem;
class ProceduralCity;

/// Top-level world system that owns terrain, streaming, and biomes.
class WorldSystem {
public:
    explicit WorldSystem(const WorldDesc& desc);
    ~WorldSystem();

    void init();
    void shutdown();

    /// Update streaming around the player position every frame.
    void update(float playerX, float playerZ, float dt);

    /// Sample terrain height at world-space (x, z).  Returns NaN if outside bounds.
    float sampleHeight(float x, float z) const;

    /// Sample biome ID at world-space (x, z).
    u8    sampleBiome(float x, float z) const;

    Terrain&        terrain();
    WorldStreamer&  streamer();
    BiomeSystem&    biomes();
    ProceduralCity& cities();

    const WorldDesc& desc() const { return m_desc; }

private:
    WorldDesc m_desc;
    std::unique_ptr<Terrain>        m_terrain;
    std::unique_ptr<WorldStreamer>  m_streamer;
    std::unique_ptr<BiomeSystem>    m_biomes;
    std::unique_ptr<ProceduralCity> m_cities;
};

} // namespace shooter
