#pragma once

#include "ecs/EntityRegistry.h"
#include "ai/SquadManager.h"
#include <vector>

namespace shooter {

/// Front-line segment along the X-axis.
struct FrontLine {
    float  xMin{0}, xMax{1000.0f};
    float  z{5000.0f};
    u8     controllerFaction{0}; ///< Faction currently controlling this segment
    float  pressure{0.5f};       ///< 0=faction 0 controls, 1=faction 1 controls
};

/// High-level large-scale battle coordinator.
///
/// Manages dynamic front lines, objective-based reinforcements,
/// and faction balance across the entire map.
class BattleSystem {
public:
    BattleSystem() = default;

    void addFrontLine(FrontLine fl)  { m_frontLines.push_back(fl); }
    void addSquadManager(SquadManager* sm) { m_squadMgrs.push_back(sm); }

    void update(float dt);

    u32 frontLineCount() const { return static_cast<u32>(m_frontLines.size()); }

    const FrontLine& frontLine(u32 i) const { return m_frontLines[i]; }

private:
    void advanceFrontLines(float dt);
    void callReinforcements();

    std::vector<FrontLine>    m_frontLines;
    std::vector<SquadManager*> m_squadMgrs;
    float                     m_reinforceTimer{0};
};

} // namespace shooter
