#pragma once

#include "core/Platform.h"
#include <cmath>
#include <array>

namespace shooter {

/// A single IK target (end-effector goal).
struct IkTarget {
    float goalX{0}, goalY{0}, goalZ{0};
    float poleX{0}, poleY{1}, poleZ{0}; ///< Pole vector hint
    u32   rootBoneIndex{UINT32_MAX};
    u32   tipBoneIndex{UINT32_MAX};
    float weight{1.0f};
};

/// Two-bone analytical IK solver (limbs: arm, leg).
///
/// Uses the law of cosines to compute joint bend angle then aligns
/// the chain with the pole vector.
class InverseKinematics {
public:
    static constexpr u32 MAX_CHAIN = 16;

    /// Solve a two-bone IK chain.
    /// \p root/\p mid/\p tip are bone positions (in/out).
    /// Returns false if target is unreachable.
    static bool solveTwoBone(float rootX,  float rootY,  float rootZ,
                              float& midX,  float& midY,  float& midZ,
                              float& tipX,  float& tipY,  float& tipZ,
                              float  goalX, float  goalY, float  goalZ,
                              float  boneALen, float boneBLen);

    /// FABRIK solver for multi-bone chains (up to MAX_CHAIN links).
    static bool solveFabrik(std::array<float, MAX_CHAIN * 3>& positions,
                             u32 chainLength,
                             float goalX, float goalY, float goalZ,
                             float tolerance = 0.001f,
                             u32   maxIterations = 10);
};

} // namespace shooter
