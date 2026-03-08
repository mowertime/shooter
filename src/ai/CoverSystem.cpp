#include "ai/CoverSystem.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace shooter {

void CoverSystem::addCoverPoint(CoverPoint cp) {
    m_points.push_back(cp);
}

CoverPoint* CoverSystem::findBestCover(float qx, float qz,
                                        float threatX, float threatZ,
                                        float maxSearchRadius) {
    CoverPoint* best    = nullptr;
    float       bestScore = -std::numeric_limits<float>::max();

    for (auto& cp : m_points) {
        if (cp.occupied) continue;
        float dx   = cp.x - qx;
        float dz   = cp.z - qz;
        float dist = std::sqrt(dx * dx + dz * dz);
        if (dist > maxSearchRadius) continue;

        // Prefer cover that faces away from the threat.
        float tdx  = cp.x - threatX;
        float tdz  = cp.z - threatZ;
        float tlen = std::sqrt(tdx * tdx + tdz * tdz);
        if (tlen < 1e-4f) tlen = 1e-4f;
        float dot   = (cp.normalX * tdx / tlen) + (cp.normalZ * tdz / tlen);
        float score = dot * cp.quality - dist * 0.01f;

        if (score > bestScore) {
            bestScore = score;
            best      = &cp;
        }
    }
    return best;
}

bool CoverSystem::occupy(CoverPoint* cp, u32 agentId) {
    if (!cp || cp->occupied) return false;
    cp->occupied    = true;
    cp->occupantId  = agentId;
    return true;
}

void CoverSystem::release(u32 agentId) {
    for (auto& cp : m_points) {
        if (cp.occupantId == agentId) {
            cp.occupied   = false;
            cp.occupantId = UINT32_MAX;
        }
    }
}

} // namespace shooter
