#pragma once

// ---------------------------------------------------------------------------
// AI.h — AI manager: perception, LOD scheduling, and squad management.
// ---------------------------------------------------------------------------

#include <vector>
#include <unordered_map>
#include <memory>

#include "engine/platform/Platform.h"
#include "engine/ecs/ECS.h"
#include "engine/physics/Physics.h"
#include "BehaviorTree.h"
#include "SquadAI.h"

namespace shooter {

// ---------------------------------------------------------------------------
// AIPerception — sensory cones for a single AI agent.
// ---------------------------------------------------------------------------
struct AIPerception {
    // Sight
    f32 sightRange{80.f};       ///< metres
    f32 sightFOVDeg{120.f};     ///< horizontal field of view
    f32 sightHeightFOVDeg{60.f};

    // Hearing — omnidirectional but range-limited
    f32 hearingRange{40.f};

    // Smell (unused in most scenarios but included for stealth builds)
    f32 smellRange{10.f};

    // State
    bool canSeeEnemy{false};
    bool canHearEnemy{false};
    EntityID lastKnownEnemy{INVALID_ENTITY};
    Vec3     lastKnownEnemyPos{};
    f32      timeSinceSaw{999.f};   ///< seconds
};

// ---------------------------------------------------------------------------
// AILODLevel — controls simulation fidelity based on distance to player.
// Far-away units run cheaper logic to save CPU.
// ---------------------------------------------------------------------------
enum class AILODLevel {
    Full,        ///< full behavior tree + perception + animation
    Reduced,     ///< simplified perception, fewer BT ticks per second
    Dormant,     ///< only position update; no perception or BT
};

// ---------------------------------------------------------------------------
// AIAgent — all runtime state for one AI-controlled entity.
// ---------------------------------------------------------------------------
struct AIAgent {
    EntityID     entityID{INVALID_ENTITY};
    u32          squadID{0};
    u32          teamID{0};
    AIPerception perception;
    AILODLevel   lod{AILODLevel::Full};
    f32          distanceToPlayer{0.f};
    BehaviorTree behaviorTree;
    f32          tickAccumulator{0.f}; ///< for rate-limiting BT ticks
};

// ===========================================================================
// AIManager — owns all agents and squads; ticks them with LOD scheduling.
// ===========================================================================
class AIManager {
public:
    static constexpr f32 FULL_AI_RADIUS    = 300.f;  ///< metres
    static constexpr f32 REDUCED_AI_RADIUS = 1000.f;

    AIManager()  = default;
    ~AIManager() = default;

    bool initialize(u32 maxAgents);
    void shutdown();

    /// Called every frame.  playerPos used for LOD distance calculation.
    void update(ECSWorld& world, f32 dt, Vec3 playerPos);

    // ---- Agent management -------------------------------------------------
    EntityID spawnAgent(ECSWorld& world, Vec3 position, u32 teamID = 1);
    void     despawnAgent(ECSWorld& world, EntityID id);
    void     assignToSquad(EntityID agentID, u32 squadID);

    // ---- Squad management -------------------------------------------------
    u32  createSquad(std::string name, u32 teamID);
    Squad* getSquad(u32 squadID);
    void   issueSquadOrder(u32 squadID, Tactic tactic, Vec3 objective = {});

    [[nodiscard]] u32 agentCount()  const { return static_cast<u32>(m_agents.size()); }
    [[nodiscard]] u32 squadCount()  const { return static_cast<u32>(m_squads.size()); }

private:
    void updatePerception(AIAgent& agent, ECSWorld& world);
    void updateLOD(AIAgent& agent, Vec3 playerPos);
    void buildDefaultBehaviorTree(AIAgent& agent);

    std::unordered_map<EntityID, AIAgent> m_agents;
    std::unordered_map<u32, Squad>        m_squads;
    CoverSystem                           m_coverSystem;

    u32 m_nextSquadID{1};
};

} // namespace shooter
