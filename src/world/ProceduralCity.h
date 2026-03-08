#pragma once

#include "core/Platform.h"
#include <vector>
#include <string>

namespace shooter {

/// Represents a procedurally generated city block.
struct CityBlock {
    float   x{0}, z{0};        ///< Centre position (world-space)
    float   width{50.0f};      ///< metres
    float   depth{50.0f};
    u32     buildingCount{4};
    float   avgBuildingHeight{12.0f};
    bool    isIntersection{false};
};

/// Procedural large-scale city generator.
///
/// Generates a road network using a grid-based L-system, then places
/// building footprints, parks, and landmarks using WFC-style rules.
class ProceduralCity {
public:
    void generate(float centerX, float centerZ,
                  float radiusM, u32 seed,
                  u32 targetBlockCount = 500);

    const std::vector<CityBlock>& blocks() const { return m_blocks; }
    u32 blockCount() const { return static_cast<u32>(m_blocks.size()); }

private:
    std::vector<CityBlock> m_blocks;
};

} // namespace shooter
