#pragma once

#include "core/Platform.h"
#include <vector>

namespace shooter {

/// A cover point that agents can occupy.
struct CoverPoint {
    float x{0}, y{0}, z{0};     ///< World-space position
    float normalX{0}, normalZ{1}; ///< Facing direction (away from cover)
    float quality{1.0f};        ///< 0..1 quality rating
    bool  occupied{false};
    u32   occupantId{UINT32_MAX};
};

/// Spatial database of cover points.
///
/// Cover points are generated offline from navmesh data or placed by
/// the level designer.  At runtime agents query for the nearest
/// available cover given a threat direction.
class CoverSystem {
public:
    void addCoverPoint(CoverPoint cp);

    /// Find the best unoccupied cover point near (qx, qz) that faces away
    /// from threat direction (tx, tz).  Returns nullptr if none available.
    CoverPoint* findBestCover(float qx, float qz,
                               float threatX, float threatZ,
                               float maxSearchRadius = 30.0f);

    /// Mark a cover point as occupied by \p agentId.
    bool occupy(CoverPoint* cp, u32 agentId);

    /// Release cover previously occupied by \p agentId.
    void release(u32 agentId);

    u32 coverPointCount() const { return static_cast<u32>(m_points.size()); }

private:
    std::vector<CoverPoint> m_points;
};

} // namespace shooter
