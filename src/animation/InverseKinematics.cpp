#include "animation/InverseKinematics.h"
#include <cmath>
#include <algorithm>

namespace shooter {

bool InverseKinematics::solveTwoBone(
        float  rootX,  float  rootY,  float  rootZ,
        float& midX,   float& midY,   float& midZ,
        float& tipX,   float& tipY,   float& tipZ,
        float  goalX,  float  goalY,  float  goalZ,
        float  boneALen, float boneBLen) {

    // Vector from root to goal.
    float dx = goalX - rootX;
    float dy = goalY - rootY;
    float dz = goalZ - rootZ;
    float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (dist < 1e-5f) return false;

    // Clamp goal within reach.
    float maxReach = boneALen + boneBLen;
    if (dist > maxReach) {
        dist = maxReach;
        dx *= maxReach / dist; dy *= maxReach / dist; dz *= maxReach / dist;
    }

    // Law of cosines: angle at root.
    float cosA = (boneALen * boneALen + dist * dist - boneBLen * boneBLen)
               / (2.0f * boneALen * dist);
    cosA = std::clamp(cosA, -1.0f, 1.0f);
    float sinA = std::sqrt(1.0f - cosA * cosA);

    // Place mid-bone (simplified: bend in XY plane of root→goal).
    float perpX = -dy / dist, perpY = dx / dist, perpZ = 0.0f;
    midX = rootX + (dx / dist) * boneALen * cosA + perpX * boneALen * sinA;
    midY = rootY + (dy / dist) * boneALen * cosA + perpY * boneALen * sinA;
    midZ = rootZ + (dz / dist) * boneALen * cosA + perpZ * boneALen * sinA;
    tipX = rootX + dx;
    tipY = rootY + dy;
    tipZ = rootZ + dz;
    return true;
}

bool InverseKinematics::solveFabrik(
        std::array<float, MAX_CHAIN * 3>& positions,
        u32 chainLength,
        float goalX, float goalY, float goalZ,
        float tolerance, u32 maxIterations) {
    if (chainLength < 2 || chainLength > MAX_CHAIN) return false;

    // Pre-compute segment lengths.
    std::array<float, MAX_CHAIN> lengths{};
    for (u32 i = 0; i + 1 < chainLength; ++i) {
        float bx = positions[(i+1)*3]   - positions[i*3];
        float by = positions[(i+1)*3+1] - positions[i*3+1];
        float bz = positions[(i+1)*3+2] - positions[i*3+2];
        lengths[i] = std::sqrt(bx*bx + by*by + bz*bz);
    }

    float rootX = positions[0], rootY = positions[1], rootZ = positions[2];

    for (u32 iter = 0; iter < maxIterations; ++iter) {
        // Forward pass: tip → root.
        positions[(chainLength-1)*3]   = goalX;
        positions[(chainLength-1)*3+1] = goalY;
        positions[(chainLength-1)*3+2] = goalZ;
        for (i32 i = static_cast<i32>(chainLength) - 2; i >= 0; --i) {
            float bx = positions[i*3]   - positions[(i+1)*3];
            float by = positions[i*3+1] - positions[(i+1)*3+1];
            float bz = positions[i*3+2] - positions[(i+1)*3+2];
            float len = std::sqrt(bx*bx + by*by + bz*bz);
            if (len < 1e-6f) len = 1e-6f;
            float scale = lengths[static_cast<u32>(i)] / len;
            positions[i*3]   = positions[(i+1)*3]   + bx * scale;
            positions[i*3+1] = positions[(i+1)*3+1] + by * scale;
            positions[i*3+2] = positions[(i+1)*3+2] + bz * scale;
        }
        // Backward pass: root → tip.
        positions[0] = rootX; positions[1] = rootY; positions[2] = rootZ;
        for (u32 i = 0; i + 1 < chainLength; ++i) {
            float bx = positions[(i+1)*3]   - positions[i*3];
            float by = positions[(i+1)*3+1] - positions[i*3+1];
            float bz = positions[(i+1)*3+2] - positions[i*3+2];
            float len = std::sqrt(bx*bx + by*by + bz*bz);
            if (len < 1e-6f) len = 1e-6f;
            float scale = lengths[i] / len;
            positions[(i+1)*3]   = positions[i*3]   + bx * scale;
            positions[(i+1)*3+1] = positions[i*3+1] + by * scale;
            positions[(i+1)*3+2] = positions[i*3+2] + bz * scale;
        }
        // Check convergence.
        float ex = positions[(chainLength-1)*3]   - goalX;
        float ey = positions[(chainLength-1)*3+1] - goalY;
        float ez = positions[(chainLength-1)*3+2] - goalZ;
        if (ex*ex + ey*ey + ez*ez < tolerance * tolerance) return true;
    }
    return false;
}

} // namespace shooter
