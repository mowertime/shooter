#pragma once

#include "core/Platform.h"
#include <vector>

namespace shooter {

/// Sight / hearing / bullet-crack perception events.
enum class StimulusType : u8 {
    Visual,
    Auditory,
    BulletCrack,
    Explosion,
    Footstep,
};

struct PerceptionEvent {
    StimulusType type;
    u32          sourceEntityId{UINT32_MAX};
    float        px{0}, py{0}, pz{0}; ///< World-space origin
    float        intensity{1.0f};     ///< 0..1 scaled by distance/occlusion
};

/// Per-agent perception state updated by the PerceptionSystem.
struct PerceptionState {
    bool  canSeeEnemy{false};
    float lastSeenEnemyX{0}, lastSeenEnemyY{0}, lastSeenEnemyZ{0};
    float lastSeenTime{-9999.0f};   ///< Game-time of last visual contact
    float lastHeardTime{-9999.0f};  ///< Game-time of last audio stimulus
    u32   primaryThreatId{UINT32_MAX};
};

/// Drives AI awareness using line-of-sight raycasting and audio propagation.
class PerceptionSystem {
public:
    /// Broadcast a stimulus to all agents within range.
    void broadcastStimulus(const PerceptionEvent& event,
                           float radius, float gameTime);

    /// Update sight checks for all registered agents.
    void updateSight(float gameTime);

    u32 registerAgent(u32 entityId);
    void unregisterAgent(u32 entityId);

    const PerceptionState* getState(u32 entityId) const;

private:
    struct AgentEntry {
        u32             entityId;
        PerceptionState state;
    };
    std::vector<AgentEntry> m_agents;
};

} // namespace shooter
