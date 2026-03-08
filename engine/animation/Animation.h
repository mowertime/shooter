#pragma once

// ---------------------------------------------------------------------------
// Animation.h — Skeletal animation, blend trees, IK, and ragdolls.
// ---------------------------------------------------------------------------

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <array>

#include "engine/platform/Platform.h"
#include "engine/physics/Physics.h"  // Vec3

namespace shooter {

// ---------------------------------------------------------------------------
// Math helpers (minimal — a real engine would use glm / DirectXMath)
// ---------------------------------------------------------------------------
struct Quat { f32 x{0}, y{0}, z{0}, w{1}; };

struct Mat4 {
    f32 m[16]{};
    static Mat4 identity() {
        Mat4 r;
        r.m[0]=r.m[5]=r.m[10]=r.m[15]=1.f;
        return r;
    }
};

// ---------------------------------------------------------------------------
// Bone
// ---------------------------------------------------------------------------
struct Bone {
    std::string name;
    i32         parentIndex{-1};  ///< -1 = root

    Vec3 localPosition{};
    Quat localRotation{};
    Vec3 localScale{1,1,1};

    Mat4 worldTransform{Mat4::identity()};
    Mat4 inverseBindPose{Mat4::identity()}; ///< T-pose inverse for skinning
};

// ---------------------------------------------------------------------------
// Skeleton — a hierarchy of bones.
// ---------------------------------------------------------------------------
class Skeleton {
public:
    u32  addBone(Bone bone);
    [[nodiscard]] Bone*       getBone(u32 index);
    [[nodiscard]] const Bone* getBone(u32 index) const;
    [[nodiscard]] i32         findBone(const std::string& name) const;
    [[nodiscard]] u32         boneCount() const { return static_cast<u32>(m_bones.size()); }

    /// Recalculate world transforms from local transforms (parent → child).
    void updateWorldTransforms();

private:
    std::vector<Bone> m_bones;
};

// ---------------------------------------------------------------------------
// Keyframe — one sample in an animation clip for a single bone.
// ---------------------------------------------------------------------------
struct Keyframe {
    f32  time{0};
    Vec3 position{};
    Quat rotation{};
    Vec3 scale{1,1,1};
};

/// Per-bone channel inside an AnimationClip.
struct BoneChannel {
    u32                  boneIndex{0};
    std::vector<Keyframe> keyframes;
};

// ---------------------------------------------------------------------------
// AnimationClip — one named animation (walk, run, aim, reload, …).
// ---------------------------------------------------------------------------
struct AnimationClip {
    std::string              name;
    f32                      duration{0};   ///< seconds
    f32                      ticksPerSecond{30.f};
    bool                     looping{true};
    std::vector<BoneChannel> channels;

    /// Sample the clip at time t (seconds) into the pose arrays.
    void sample(f32 t, std::vector<Vec3>& outPositions,
                std::vector<Quat>& outRotations,
                std::vector<Vec3>& outScales) const;
};

// ---------------------------------------------------------------------------
// AnimationBlendNode — node in a blend tree.
// ---------------------------------------------------------------------------
struct BlendNode {
    std::string clipName;
    f32         weight{1.f};
    f32         playbackRate{1.f};
    f32         currentTime{0.f};
};

// ---------------------------------------------------------------------------
// AnimationStateMachine — transitions between animation states.
// ---------------------------------------------------------------------------
class AnimationStateMachine {
public:
    void addClip(const std::string& name, AnimationClip clip);
    void setCurrentClip(const std::string& name);

    /// Blend from current clip towards targetClip over blendDuration seconds.
    void crossfadeTo(const std::string& targetClip, f32 blendDuration);

    /// Advance time and compute the final pose for the skeleton.
    void update(Skeleton& skeleton, f32 dt);

private:
    std::unordered_map<std::string, AnimationClip> m_clips;
    std::string m_currentClip;
    std::string m_targetClip;
    f32         m_currentTime{0};
    f32         m_blendTime{0};
    f32         m_blendDuration{0};
};

// ---------------------------------------------------------------------------
// IKSolver — FABRIK (Forward And Backward Reaching Inverse Kinematics).
// Used for foot placement on uneven terrain and hand IK on weapons.
// ---------------------------------------------------------------------------
class IKSolver {
public:
    static constexpr u32 MAX_ITERATIONS = 10;
    static constexpr f32 TOLERANCE      = 0.01f; ///< metres

    /// Solve the IK chain from chainStart to chainEnd so that chainEnd reaches
    /// targetPos.  Modifies the bone positions in-place.
    static void solve(Skeleton& skeleton,
                      u32 chainStart, u32 chainEnd,
                      Vec3 targetPos);
};

// ---------------------------------------------------------------------------
// RagdollSystem — activates physics-driven pose when an entity dies.
// ---------------------------------------------------------------------------
class RagdollSystem {
public:
    /// Register a skeleton as ragdoll-capable.
    void registerSkeleton(EntityID entity, Skeleton* skeleton);

    /// Activate ragdoll for entity (called on death).
    /// Transfers current bone poses to physics rigid bodies.
    void activateRagdoll(EntityID entity, Vec3 impulse = {});

    void update(f32 dt);

private:
    struct RagdollInstance {
        EntityID  entityID{INVALID_ENTITY};
        Skeleton* skeleton{nullptr};
        bool      active{false};
        // TODO: array of rigid body handles, one per bone
    };

    std::unordered_map<EntityID, RagdollInstance> m_ragdolls;
};

// ---------------------------------------------------------------------------
// AnimationSystem — ECS system updating all animated entities.
// ---------------------------------------------------------------------------
class AnimationSystem {
public:
    bool initialize();
    void shutdown();

    void registerEntity(EntityID entity, Skeleton* skeleton);
    void setStateMachine(EntityID entity,
                         std::shared_ptr<AnimationStateMachine> sm);
    void update(f32 dt);

private:
    struct AnimatedEntity {
        EntityID                              id;
        Skeleton*                             skeleton{nullptr};
        std::shared_ptr<AnimationStateMachine> stateMachine;
    };

    std::unordered_map<EntityID, AnimatedEntity> m_entities;
    RagdollSystem                                 m_ragdolls;
};

} // namespace shooter
