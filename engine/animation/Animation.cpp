// ---------------------------------------------------------------------------
// Animation.cpp — Animation system implementation.
// ---------------------------------------------------------------------------

#include "engine/animation/Animation.h"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace shooter {

// ===========================================================================
// Skeleton
// ===========================================================================

u32 Skeleton::addBone(Bone bone) {
    u32 idx = static_cast<u32>(m_bones.size());
    m_bones.push_back(std::move(bone));
    return idx;
}

Bone* Skeleton::getBone(u32 index) {
    return index < m_bones.size() ? &m_bones[index] : nullptr;
}

const Bone* Skeleton::getBone(u32 index) const {
    return index < m_bones.size() ? &m_bones[index] : nullptr;
}

i32 Skeleton::findBone(const std::string& name) const {
    for (u32 i = 0; i < static_cast<u32>(m_bones.size()); ++i) {
        if (m_bones[i].name == name) return static_cast<i32>(i);
    }
    return -1;
}

void Skeleton::updateWorldTransforms() {
    // Forward pass: parents are guaranteed to have lower indices than children.
    for (auto& bone : m_bones) {
        if (bone.parentIndex < 0) {
            // Root bone: world = local
            bone.worldTransform = Mat4::identity();
            // TODO: compose bone.localPosition/Rotation/Scale into Mat4
        } else {
            // Child: world = parent.world * local
            // TODO: matrix multiplication with parent's world transform
        }
    }
}

// ===========================================================================
// AnimationClip::sample
// ===========================================================================

/// Linear interpolation helpers for the prototype.
static Vec3 lerpVec3(const Vec3& a, const Vec3& b, f32 t) {
    return { a.x + (b.x-a.x)*t, a.y + (b.y-a.y)*t, a.z + (b.z-a.z)*t };
}

static Quat slerpQuat(const Quat& a, const Quat& b, f32 t) {
    // Simplified NLERP (normalised lerp) — good enough for a prototype.
    Quat r;
    r.x = a.x + (b.x-a.x)*t;
    r.y = a.y + (b.y-a.y)*t;
    r.z = a.z + (b.z-a.z)*t;
    r.w = a.w + (b.w-a.w)*t;
    f32 len = std::sqrt(r.x*r.x + r.y*r.y + r.z*r.z + r.w*r.w);
    if (len > 1e-6f) { r.x/=len; r.y/=len; r.z/=len; r.w/=len; }
    return r;
}

void AnimationClip::sample(f32 t, std::vector<Vec3>& outPositions,
                            std::vector<Quat>& outRotations,
                            std::vector<Vec3>& outScales) const {
    // Clamp or wrap time.
    f32 localT = looping ? std::fmod(t, duration) : std::min(t, duration);

    for (const auto& ch : channels) {
        if (ch.keyframes.empty()) continue;
        if (ch.boneIndex >= outPositions.size()) continue;

        // Binary search for surrounding keyframes.
        const Keyframe* prev = &ch.keyframes.front();
        const Keyframe* next = &ch.keyframes.back();
        for (size_t i = 0; i + 1 < ch.keyframes.size(); ++i) {
            if (ch.keyframes[i].time <= localT &&
                ch.keyframes[i+1].time >= localT) {
                prev = &ch.keyframes[i];
                next = &ch.keyframes[i+1];
                break;
            }
        }

        f32 segLen = next->time - prev->time;
        f32 alpha  = segLen > 1e-6f ? (localT - prev->time) / segLen : 0.f;

        outPositions[ch.boneIndex] = lerpVec3(prev->position, next->position, alpha);
        outRotations[ch.boneIndex] = slerpQuat(prev->rotation, next->rotation, alpha);
        outScales[ch.boneIndex]    = lerpVec3(prev->scale,    next->scale,    alpha);
    }
}

// ===========================================================================
// AnimationStateMachine
// ===========================================================================

void AnimationStateMachine::addClip(const std::string& name, AnimationClip clip) {
    m_clips[name] = std::move(clip);
}

void AnimationStateMachine::setCurrentClip(const std::string& name) {
    m_currentClip = name;
    m_currentTime = 0.f;
}

void AnimationStateMachine::crossfadeTo(const std::string& targetClip, f32 blendDuration) {
    m_targetClip    = targetClip;
    m_blendTime     = 0.f;
    m_blendDuration = blendDuration;
}

void AnimationStateMachine::update(Skeleton& skeleton, f32 dt) {
    auto it = m_clips.find(m_currentClip);
    if (it == m_clips.end()) return;

    const AnimationClip& clip = it->second;
    m_currentTime += dt;

    u32 boneCount = skeleton.boneCount();
    std::vector<Vec3> positions(boneCount);
    std::vector<Quat> rotations(boneCount);
    std::vector<Vec3> scales(boneCount, Vec3{1,1,1});

    clip.sample(m_currentTime, positions, rotations, scales);

    // Handle crossfade blend.
    if (!m_targetClip.empty() && m_blendDuration > 0.f) {
        m_blendTime += dt;
        f32 alpha = std::min(m_blendTime / m_blendDuration, 1.f);

        auto tit = m_clips.find(m_targetClip);
        if (tit != m_clips.end()) {
            std::vector<Vec3> tPos(boneCount);
            std::vector<Quat> tRot(boneCount);
            std::vector<Vec3> tScl(boneCount, Vec3{1,1,1});
            tit->second.sample(m_blendTime, tPos, tRot, tScl);

            for (u32 i = 0; i < boneCount; ++i) {
                positions[i] = lerpVec3(positions[i], tPos[i], alpha);
                rotations[i] = slerpQuat(rotations[i], tRot[i], alpha);
                scales[i]    = lerpVec3(scales[i],    tScl[i], alpha);
            }
        }

        if (alpha >= 1.f) {
            m_currentClip = m_targetClip;
            m_currentTime = m_blendTime;
            m_targetClip.clear();
        }
    }

    // Apply pose to skeleton.
    for (u32 i = 0; i < boneCount; ++i) {
        if (Bone* b = skeleton.getBone(i)) {
            b->localPosition = positions[i];
            b->localRotation = rotations[i];
            b->localScale    = scales[i];
        }
    }
    skeleton.updateWorldTransforms();
}

// ===========================================================================
// IKSolver — FABRIK
// ===========================================================================

void IKSolver::solve(Skeleton& skeleton,
                     u32 chainStart, u32 chainEnd, Vec3 targetPos) {
    // Collect chain indices (start → end, parent-first).
    std::vector<u32> chain;
    for (u32 i = chainStart; i <= chainEnd; ++i) chain.push_back(i);

    if (chain.size() < 2) return;

    // Compute segment lengths.
    std::vector<f32> lengths(chain.size() - 1);
    f32 totalLength = 0.f;
    for (size_t i = 0; i + 1 < chain.size(); ++i) {
        Bone* a = skeleton.getBone(chain[i]);
        Bone* b = skeleton.getBone(chain[i+1]);
        if (!a || !b) continue;
        Vec3 diff = b->localPosition - a->localPosition;
        lengths[i] = diff.length();
        totalLength += lengths[i];
    }

    // Check reachability.
    Vec3  rootPos = skeleton.getBone(chain.front())->localPosition;
    Vec3  diff    = targetPos - rootPos;
    if (diff.length() >= totalLength) {
        // Stretch straight toward target — accumulate positions along the chain.
        Vec3 dir = diff.normalised();
        Vec3 prev = rootPos;
        for (size_t i = 1; i < chain.size(); ++i) {
            prev = prev + dir * lengths[i-1];
            Bone* b = skeleton.getBone(chain[i]);
            if (b) b->localPosition = prev;
        }
        return;
    }

    // Iterative FABRIK.
    std::vector<Vec3> positions(chain.size());
    for (size_t i = 0; i < chain.size(); ++i)
        positions[i] = skeleton.getBone(chain[i])->localPosition;

    for (u32 iter = 0; iter < MAX_ITERATIONS; ++iter) {
        // Forward pass: move end effector to target.
        positions.back() = targetPos;
        for (i32 i = static_cast<i32>(positions.size()) - 2; i >= 0; --i) {
            Vec3 d = (positions[static_cast<size_t>(i)] - positions[static_cast<size_t>(i+1)]).normalised();
            positions[static_cast<size_t>(i)] = positions[static_cast<size_t>(i+1)] + d * lengths[static_cast<size_t>(i)];
        }
        // Backward pass: pin root.
        positions.front() = rootPos;
        for (size_t i = 1; i < positions.size(); ++i) {
            Vec3 d = (positions[i] - positions[i-1]).normalised();
            positions[i] = positions[i-1] + d * lengths[i-1];
        }
        // Convergence check.
        Vec3 errVec = positions.back() - targetPos;
        if (errVec.length() < TOLERANCE) break;
    }

    // Write back to skeleton.
    for (size_t i = 0; i < chain.size(); ++i) {
        if (Bone* b = skeleton.getBone(chain[i]))
            b->localPosition = positions[i];
    }
}

// ===========================================================================
// RagdollSystem
// ===========================================================================

void RagdollSystem::registerSkeleton(EntityID entity, Skeleton* skeleton) {
    RagdollInstance inst;
    inst.entityID = entity;
    inst.skeleton = skeleton;
    m_ragdolls[entity] = std::move(inst);
}

void RagdollSystem::activateRagdoll(EntityID entity, Vec3 impulse) {
    auto it = m_ragdolls.find(entity);
    if (it == m_ragdolls.end()) return;
    it->second.active = true;
    // TODO: create one PhysicsWorld rigid body per bone, apply impulse to root.
    (void)impulse;
    std::cout << "[Ragdoll] Activated for entity " << entity << "\n";
}

void RagdollSystem::update(f32 /*dt*/) {
    for (auto& [eid, inst] : m_ragdolls) {
        if (!inst.active) continue;
        // TODO: read rigid body transforms back into skeleton bones.
    }
}

// ===========================================================================
// AnimationSystem
// ===========================================================================

bool AnimationSystem::initialize() {
    std::cout << "[Animation] System initialized\n";
    return true;
}

void AnimationSystem::shutdown() {
    m_entities.clear();
    std::cout << "[Animation] System shutdown\n";
}

void AnimationSystem::registerEntity(EntityID entity, Skeleton* skeleton) {
    AnimatedEntity ae;
    ae.id       = entity;
    ae.skeleton = skeleton;
    m_entities[entity] = std::move(ae);
    m_ragdolls.registerSkeleton(entity, skeleton);
}

void AnimationSystem::setStateMachine(EntityID entity,
    std::shared_ptr<AnimationStateMachine> sm) {
    auto it = m_entities.find(entity);
    if (it != m_entities.end()) it->second.stateMachine = std::move(sm);
}

void AnimationSystem::update(f32 dt) {
    for (auto& [eid, ae] : m_entities) {
        if (ae.stateMachine && ae.skeleton) {
            ae.stateMachine->update(*ae.skeleton, dt);
        }
    }
    m_ragdolls.update(dt);
}

} // namespace shooter
