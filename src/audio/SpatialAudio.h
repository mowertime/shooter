#pragma once

#include "audio/AudioSystem.h"
#include <algorithm>
#include <cmath>

namespace shooter {

/// Spatial audio helper utilities (panning, distance attenuation).
namespace SpatialAudio {

/// Inverse-square law attenuation clamped to [0, 1].
inline float attenuate(float distance, float maxDistance) {
    if (distance >= maxDistance) return 0.0f;
    if (distance <= 0.0f)        return 1.0f;
    float t = distance / maxDistance;
    return std::max(0.0f, 1.0f - t * t); // quadratic roll-off
}

/// Simple equal-power stereo panning.
/// \p panAngle is in radians, 0 = centre, -π/2 = hard left, +π/2 = hard right.
inline void equalPowerPan(float panAngle, float& leftGain, float& rightGain) {
    float t     = (panAngle + 1.5707963268f) / 3.1415926536f; // normalise 0..1
    leftGain    = std::cos(t * 1.5707963268f);
    rightGain   = std::sin(t * 1.5707963268f);
}

/// Compute the pan angle of a sound source relative to a listener.
/// Returns radians in [-π/2, π/2].
inline float computePanAngle(float srcX, float srcZ,
                              float lstX, float lstZ,
                              float lstFwdX, float lstFwdZ) {
    float dx     = srcX - lstX;
    float dz     = srcZ - lstZ;
    float len    = std::sqrt(dx * dx + dz * dz);
    if (len < 1e-4f) return 0.0f;
    dx /= len; dz /= len;
    // Right vector = rotate forward 90°.
    float rightX = lstFwdZ;
    float rightZ = -lstFwdX;
    float dot    = dx * rightX + dz * rightZ;
    return std::asin(std::clamp(dot, -1.0f, 1.0f));
}

} // namespace SpatialAudio
} // namespace shooter
