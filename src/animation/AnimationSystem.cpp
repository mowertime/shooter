#include "animation/AnimationSystem.h"
#include <algorithm>

namespace shooter {

u32 AnimationSystem::loadClip(AnimClip clip) {
    u32 id = static_cast<u32>(m_clips.size());
    m_clips.push_back(std::move(clip));
    return id;
}

void AnimationSystem::playClip(u32 entityId, u32 clipIndex,
                                bool loop, float blendTime) {
    EntityAnim* ea = findOrCreate(entityId);
    // Start blend: move current → blend slot, start new clip in current.
    ea->blend          = ea->current;
    ea->current.clipIndex  = clipIndex;
    ea->current.time       = 0;
    ea->current.loop       = loop;
    ea->current.playing    = true;
    ea->current.blendWeight = 0.0f;
    ea->blendTimer     = 0;
    ea->blendDuration  = blendTime;
}

void AnimationSystem::stopClip(u32 entityId) {
    EntityAnim* ea = findOrCreate(entityId);
    ea->current.playing = false;
}

void AnimationSystem::update(float dt) {
    for (auto& ea : m_animators) {
        if (!ea.current.playing) continue;
        if (ea.current.clipIndex >= m_clips.size()) continue;
        const AnimClip& clip = m_clips[ea.current.clipIndex];

        ea.current.time += dt;
        if (ea.current.time > clip.duration) {
            if (ea.current.loop) ea.current.time -= clip.duration;
            else { ea.current.time = clip.duration; ea.current.playing = false; }
        }

        // Blend weight ramp.
        if (ea.blendTimer < ea.blendDuration) {
            ea.blendTimer += dt;
            ea.current.blendWeight = std::min(1.0f, ea.blendTimer / ea.blendDuration);
        } else {
            ea.current.blendWeight = 1.0f;
        }
        // A full implementation would interpolate keyframes and write
        // the resulting bone matrix palette to ea.boneBuffer.
    }
}

const std::vector<float>* AnimationSystem::getBoneBuffer(u32 entityId) const {
    for (const auto& ea : m_animators) {
        if (ea.entityId == entityId) return &ea.boneBuffer;
    }
    return nullptr;
}

AnimationSystem::EntityAnim* AnimationSystem::findOrCreate(u32 entityId) {
    for (auto& ea : m_animators) {
        if (ea.entityId == entityId) return &ea;
    }
    m_animators.push_back({entityId, {}, {}, 0, 0.2f, {}});
    return &m_animators.back();
}

} // namespace shooter
