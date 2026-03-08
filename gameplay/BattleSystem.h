#pragma once

// ---------------------------------------------------------------------------
// BattleSystem.h — Large-scale battle management.
//
// Tracks front lines, capture objectives, reinforcement waves, and the
// overall flow of a combined-arms engagement over a 200+ km² map.
// ---------------------------------------------------------------------------

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

#include "engine/platform/Platform.h"
#include "engine/physics/Physics.h"  // Vec3
#include "engine/ai/AI.h"
#include "engine/ecs/ECS.h"
#include "Vehicle.h"

namespace shooter {

// ---------------------------------------------------------------------------
// BattleObjective — a capture point that changes faction control.
// ---------------------------------------------------------------------------
enum class ObjectiveType { CapturePoint, FOB, Airfield, SupplyDepot };

struct BattleObjective {
    u32           objectiveID{0};
    std::string   name;
    ObjectiveType type{ObjectiveType::CapturePoint};
    Vec3          position{};
    f32           captureRadius{50.f};  ///< metres

    u32  controllingTeam{0};  ///< 0 = neutral
    f32  captureProgress{0.f};///< [-1,1]: -1 = fully team2, +1 = fully team1
    bool isContested{false};

    void update(f32 dt, u32 attackingTeam, u32 defendersNear, u32 attackersNear);
};

// ---------------------------------------------------------------------------
// FrontLine — a dynamic polyline representing the contact line between forces.
// Updated each frame from the positions of forward-most units per faction.
// ---------------------------------------------------------------------------
struct FrontLine {
    u32              teamID{0};
    std::vector<Vec3> points;    ///< ordered world-space positions

    /// Rebuild the front line from the leading edge of 'teamID' units.
    void rebuild(const std::vector<Vec3>& unitPositions);

    /// Returns true if 'pos' is behind the front line for this team.
    [[nodiscard]] bool isRear(Vec3 pos) const;
};

// ---------------------------------------------------------------------------
// ReinforcementWave — a spawning wave of AI squads for one faction.
// ---------------------------------------------------------------------------
struct ReinforcementWave {
    u32         teamID{0};
    u32         squadCount{3};
    u32         soldiersPerSquad{8};
    Vec3        spawnZone{};     ///< approximate spawn area centre
    f32         spawnRadius{200.f};
    f32         cooldownSeconds{120.f};
    f32         cooldownRemaining{0.f};
    bool        isReady() const { return cooldownRemaining <= 0.f; }
};

// ---------------------------------------------------------------------------
// BattleScore — tracks kills, objectives, and time to determine winner.
// ---------------------------------------------------------------------------
struct BattleScore {
    u32 teamID{0};
    u32 kills{0};
    u32 deaths{0};
    u32 objectivesCaptured{0};
    f32 controlTime{0.f};   ///< seconds of map control
};

// ===========================================================================
// BattleSystem
// ===========================================================================
class BattleSystem {
public:
    BattleSystem()  = default;
    ~BattleSystem() = default;

    bool initialize(ECSWorld* world, AIManager* ai, VehicleSystem* vehicles);
    void shutdown();

    /// Main update — call once per game tick.
    void update(f32 dt);

    // ---- Objective management ---------------------------------------------
    u32  addObjective(BattleObjective obj);
    void captureObjective(u32 objID, u32 teamID);
    [[nodiscard]] BattleObjective* getObjective(u32 id);
    [[nodiscard]] u32 objectiveCount() const {
        return static_cast<u32>(m_objectives.size());
    }

    // ---- Reinforcements ---------------------------------------------------
    void addReinforcementWave(ReinforcementWave wave);
    void triggerWave(u32 index);

    // ---- Scoring ----------------------------------------------------------
    [[nodiscard]] const BattleScore& getScore(u32 teamID) const;
    void reportKill(u32 killerTeam, u32 victimTeam);

    // ---- Front line -------------------------------------------------------
    [[nodiscard]] const FrontLine& getFrontLine(u32 teamID) const;

    /// Returns true if one team has won (all objectives captured or tickets = 0).
    [[nodiscard]] bool isBattleOver() const;
    [[nodiscard]] u32  winningTeam()  const { return m_winningTeam; }

    // ---- Event callbacks --------------------------------------------------
    using ObjectiveCapturedCB = std::function<void(u32 objID, u32 teamID)>;
    void setOnObjectiveCaptured(ObjectiveCapturedCB cb) { m_onCapture = std::move(cb); }

private:
    void updateObjectives(f32 dt);
    void updateReinforcements(f32 dt);
    void updateFrontLines();
    void checkWinCondition();

    ECSWorld*      m_world{nullptr};
    AIManager*     m_ai{nullptr};
    VehicleSystem* m_vehicles{nullptr};

    std::unordered_map<u32, BattleObjective>  m_objectives;
    std::vector<ReinforcementWave>             m_waves;
    std::unordered_map<u32, BattleScore>       m_scores;
    std::unordered_map<u32, FrontLine>         m_frontLines;

    u32  m_nextObjID{1};
    u32  m_winningTeam{0};
    bool m_battleOver{false};

    ObjectiveCapturedCB m_onCapture;
};

} // namespace shooter
