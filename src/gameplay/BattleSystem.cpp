#include "gameplay/BattleSystem.h"
#include "core/Logger.h"
#include <algorithm>

namespace shooter {

void BattleSystem::update(float dt) {
    advanceFrontLines(dt);
    m_reinforceTimer += dt;
    if (m_reinforceTimer > 30.0f) {
        m_reinforceTimer = 0.0f;
        callReinforcements();
    }
}

void BattleSystem::advanceFrontLines(float dt) {
    for (auto& fl : m_frontLines) {
        // Simulate front-line pressure change as a random walk
        // (in a real game this reflects actual unit presence).
        fl.pressure = std::clamp(fl.pressure + (dt * 0.001f), 0.0f, 1.0f);
        if (fl.pressure > 0.5f)
            fl.controllerFaction = 1;
        else
            fl.controllerFaction = 0;
    }
}

void BattleSystem::callReinforcements() {
    // Count active squads per faction and reinforce the weaker side.
    LOG_INFO("BattleSystem", "Calling reinforcements");
}

} // namespace shooter
