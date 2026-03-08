#pragma once

#include "core/Platform.h"
#include <vector>
#include <string>

namespace shooter {

/// A single keyframe: bone transforms at a given time.
struct AnimKeyframe {
    float time{0};                 ///< seconds
    std::vector<float> boneData;  ///< packed TRS per bone (10 floats each)
};

/// A named animation clip.
struct AnimClip {
    std::string             name;
    float                   duration{1.0f};
    bool                    loop{false};
    std::vector<AnimKeyframe> keyframes;
};

/// Per-entity animation instance.
struct AnimatorState {
    u32   clipIndex{UINT32_MAX};
    float time{0};
    float blendWeight{1.0f};
    bool  playing{false};
    bool  loop{false};
};

/// Skeletal animation system.
///
/// Blends up to 2 clips per entity (cross-fade) and writes the
/// final pose into a flat bone transform buffer for the GPU skinning shader.
class AnimationSystem {
public:
    u32  loadClip(AnimClip clip);

    void playClip  (u32 entityId, u32 clipIndex, bool loop = false, float blendTime = 0.2f);
    void stopClip  (u32 entityId);
    void update    (float dt);

    /// Returns the current bone-transform buffer for \p entityId.
    const std::vector<float>* getBoneBuffer(u32 entityId) const;

    u32 clipCount() const { return static_cast<u32>(m_clips.size()); }

private:
    std::vector<AnimClip>      m_clips;

    struct EntityAnim {
        u32              entityId;
        AnimatorState    current;
        AnimatorState    blend;
        float            blendTimer{0};
        float            blendDuration{0.2f};
        std::vector<float> boneBuffer;
    };
    std::vector<EntityAnim> m_animators;

    EntityAnim* findOrCreate(u32 entityId);
};

} // namespace shooter
