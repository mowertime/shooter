#pragma once

// ---------------------------------------------------------------------------
// SquadAI.h — Squad-level coordination layer above individual behavior trees.
//
// A Squad contains 4-12 soldiers led by a SquadLeader.  The leader issues
// tactical orders; members execute those orders via their behavior trees.
// ---------------------------------------------------------------------------

#include <vector>
#include <string>
#include <unordered_map>

#include "engine/platform/Platform.h"
#include "engine/ecs/ECS.h"
#include "engine/physics/Physics.h"  // Vec3

namespace shooter {

// ---------------------------------------------------------------------------
// Tactics — high-level orders a squad leader can issue.
// ---------------------------------------------------------------------------
enum class Tactic {
    Idle,       ///< hold position
    Patrol,     ///< move along a waypoint route
    Attack,     ///< engage the primary threat
    Defend,     ///< hold and fire from cover
    Flank,      ///< move to envelop the enemy flank
    Retreat,    ///< fall back to a rally point
    Suppress,   ///< keep the enemy's heads down
    Assault,    ///< close-quarter assault on a position
};

// ---------------------------------------------------------------------------
// Formation — spatial arrangement of squad members.
// ---------------------------------------------------------------------------
enum class Formation {
    Line,       ///< shoulder-to-shoulder — good for open terrain assaults
    Wedge,      ///< V-shape — mutual cover, common approach
    Column,     ///< single file — for narrow terrain / stealth
    Echelon,    ///< diagonal — one-side flank protection
    Circle,     ///< all-round defence
};

// ---------------------------------------------------------------------------
// Squad — a group of AI combatants that cooperate.
// ---------------------------------------------------------------------------
struct Squad {
    u32              squadID{0};
    std::string      name;
    EntityID         leaderID{INVALID_ENTITY};
    std::vector<EntityID> memberIDs;

    Tactic    currentTactic{Tactic::Idle};
    Formation currentFormation{Formation::Wedge};
    Vec3      objectivePosition{};   ///< world-space attack/defend point
    Vec3      rallyPosition{};       ///< fallback point used during Retreat

    u32  teamID{0};          ///< faction identifier (0 = player team, 1+ = AI factions)
    bool isAlive{true};

    /// Quick check: returns true if the squad still has at least one living member.
    [[nodiscard]] bool hasLivingMembers() const { return !memberIDs.empty(); }
};

// ---------------------------------------------------------------------------
// CoverPoint — a position that provides ballistic cover from a direction.
// ---------------------------------------------------------------------------
struct CoverPoint {
    Vec3 position;
    Vec3 facingDirection; ///< direction from which this cover blocks shots
    f32  quality{1.f};   ///< [0,1] — 1 = full hard cover, 0 = no cover
    bool isOccupied{false};
    EntityID occupantID{INVALID_ENTITY};
};

// ---------------------------------------------------------------------------
// CoverSystem — indexes cover points in the world for fast lookup.
// ---------------------------------------------------------------------------
class CoverSystem {
public:
    void addCoverPoint(CoverPoint cp) {
        m_coverPoints.push_back(std::move(cp));
    }

    /// Find the best unoccupied cover point within 'radius' metres of 'pos'.
    [[nodiscard]] CoverPoint* findBestCover(Vec3 pos, f32 radius, Vec3 threatDir) {
        CoverPoint* best    = nullptr;
        f32         bestScore = -1.f;

        for (auto& cp : m_coverPoints) {
            if (cp.isOccupied) continue;
            Vec3 diff  = cp.position - pos;
            f32  dist  = diff.length();
            if (dist > radius) continue;

            // Score: high quality + facing away from threat = good cover.
            f32 alignment = threatDir.dot(cp.facingDirection); // [-1,1]
            f32 score     = cp.quality * (1.f + alignment) * 0.5f;
            if (score > bestScore) {
                bestScore = score;
                best      = &cp;
            }
        }
        return best;
    }

    void markOccupied(CoverPoint* cp, EntityID occupant) {
        if (!cp) return;
        cp->isOccupied  = true;
        cp->occupantID  = occupant;
    }

    void releaseCover(EntityID occupant) {
        for (auto& cp : m_coverPoints) {
            if (cp.occupantID == occupant) {
                cp.isOccupied = false;
                cp.occupantID = INVALID_ENTITY;
            }
        }
    }

private:
    std::vector<CoverPoint> m_coverPoints;
};

} // namespace shooter
