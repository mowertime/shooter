#include "world/ProceduralCity.h"
#include "core/Logger.h"
#include <cmath>

namespace shooter {

void ProceduralCity::generate(float centerX, float centerZ,
                               float radiusM, u32 seed,
                               u32 targetBlockCount) {
    m_blocks.clear();
    m_blocks.reserve(targetBlockCount);

    float blockSize  = 80.0f; // metres per block
    float streetSize = 15.0f;
    float step       = blockSize + streetSize;
    i32   gridRadius = static_cast<i32>(std::ceil(radiusM / step));

    u32 count = 0;
    for (i32 gz = -gridRadius; gz <= gridRadius && count < targetBlockCount; ++gz) {
        for (i32 gx = -gridRadius; gx <= gridRadius && count < targetBlockCount; ++gx) {
            float wx = centerX + static_cast<float>(gx) * step;
            float wz = centerZ + static_cast<float>(gz) * step;
            float dx = wx - centerX, dz = wz - centerZ;
            if (dx * dx + dz * dz > radiusM * radiusM) continue;

            // Vary building parameters using a deterministic hash.
            u32 h = static_cast<u32>(gx * 73856093) ^ static_cast<u32>(gz * 19349663) ^ seed;
            h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
            float rand01 = static_cast<float>(h & 0xFFFFu) / 65535.0f;

            CityBlock blk;
            blk.x                = wx;
            blk.z                = wz;
            blk.width            = blockSize * (0.7f + rand01 * 0.3f);
            blk.depth            = blockSize * (0.7f + rand01 * 0.3f);
            blk.buildingCount    = 2 + (h & 3);
            blk.avgBuildingHeight = 8.0f + rand01 * 30.0f;
            blk.isIntersection   = (gx % 4 == 0 && gz % 4 == 0);
            m_blocks.push_back(blk);
            ++count;
        }
    }
    LOG_INFO("ProceduralCity", "Generated %u city blocks around (%.0f, %.0f)",
             count, centerX, centerZ);
}

} // namespace shooter
