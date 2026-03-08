#pragma once

#include "ai/AIController.h"
#include <vector>

namespace shooter {

/// Squad formation types.
enum class FormationType : u8 {
    Wedge,
    Line,
    Column,
    Bounding, ///< Bounding overwatch
};

/// Squad tactical state.
enum class SquadState : u8 {
    Idle,
    Patrol,
    Assault,
    Defend,
    Withdraw,
    Suppressing,
};

/// A squad groups up to 12 AI controllers and applies shared tactical logic.
class Squad {
public:
    static constexpr u32 MAX_MEMBERS = 12;

    explicit Squad(u32 squadId, u8 factionId);

    void addMember(u32 controllerId);
    void removeMember(u32 controllerId);

    /// Update squad-level decision making (orders members to positions).
    void update(AIControllerPool& pool, float dt);

    void setState(SquadState s)        { m_state = s;     }
    void setFormation(FormationType f) { m_formation = f; }

    void setObjective(float x, float y, float z) {
        m_objX = x; m_objY = y; m_objZ = z;
    }

    u32        squadId()   const { return m_squadId;   }
    u8         factionId() const { return m_factionId; }
    SquadState state()     const { return m_state;     }
    u32        memberCount() const;

private:
    u32           m_squadId;
    u8            m_factionId;
    SquadState    m_state{SquadState::Idle};
    FormationType m_formation{FormationType::Wedge};
    float         m_objX{0}, m_objY{0}, m_objZ{0};

    std::vector<u32> m_memberIds; ///< Controller indices
};

/// Global squad manager.
class SquadManager {
public:
    u32    createSquad(u8 factionId);
    Squad* getSquad(u32 squadId);
    void   updateAll(AIControllerPool& pool, float dt);
    u32    squadCount() const { return static_cast<u32>(m_squads.size()); }

private:
    std::vector<Squad> m_squads;
};

} // namespace shooter
