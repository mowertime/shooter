// ---------------------------------------------------------------------------
// BattleSystem.cpp — Large-scale battle management implementation.
// ---------------------------------------------------------------------------

#include "gameplay/BattleSystem.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <numeric>

namespace shooter {

// ===========================================================================
// BattleObjective
// ===========================================================================

void BattleObjective::update(f32 dt, u32 /*attackingTeam*/,
                              u32 defendersNear, u32 attackersNear) {
    isContested = (defendersNear > 0 && attackersNear > 0);

    if (isContested) return; // No progress while contested

    // Capture rate: +1 progress per second per attacker (capped).
    constexpr f32 captureRate = 0.05f; // progress units per soldier per second
    f32 delta = static_cast<f32>(attackersNear) * captureRate * dt;
    f32 defend= static_cast<f32>(defendersNear) * captureRate * dt;
    captureProgress += (delta - defend);
    captureProgress  = std::clamp(captureProgress, -1.f, 1.f);

    if      (captureProgress >=  1.f) controllingTeam = 1;
    else if (captureProgress <= -1.f) controllingTeam = 2;
    else                              controllingTeam = 0; // neutral/transitioning
}

// ===========================================================================
// FrontLine
// ===========================================================================

void FrontLine::rebuild(const std::vector<Vec3>& unitPositions) {
    // Simple approach: project all positions onto the X axis, sort, and take
    // the forward quartile as the front line polyline.
    if (unitPositions.empty()) { points.clear(); return; }

    // Find bounding box and estimate forward direction (naively along X).
    std::vector<Vec3> sorted = unitPositions;
    std::sort(sorted.begin(), sorted.end(),
        [](const Vec3& a, const Vec3& b){ return a.x > b.x; }); // descending X

    u32 frontCount = std::max(1u, static_cast<u32>(sorted.size()) / 4);
    points.assign(sorted.begin(), sorted.begin() + frontCount);
}

bool FrontLine::isRear(Vec3 pos) const {
    if (points.empty()) return true;
    // Compute average X of the front line.
    f32 avgX = 0.f;
    for (auto& p : points) avgX += p.x;
    avgX /= static_cast<f32>(points.size());
    return pos.x < avgX; // behind the front
}

// ===========================================================================
// BattleSystem
// ===========================================================================

bool BattleSystem::initialize(ECSWorld* world, AIManager* ai,
                               VehicleSystem* vehicles) {
    m_world    = world;
    m_ai       = ai;
    m_vehicles = vehicles;

    // Initialise scores for two default teams.
    m_scores[1] = BattleScore{1};
    m_scores[2] = BattleScore{2};

    std::cout << "[BattleSystem] Initialized\n";
    return true;
}

void BattleSystem::shutdown() {
    m_objectives.clear();
    m_waves.clear();
    m_scores.clear();
    m_frontLines.clear();
    std::cout << "[BattleSystem] Shutdown\n";
}

void BattleSystem::update(f32 dt) {
    if (m_battleOver) return;

    updateObjectives(dt);
    updateReinforcements(dt);
    updateFrontLines();
    checkWinCondition();

    // Accumulate control time for the team that holds the majority.
    u32 team1Objs = 0, team2Objs = 0;
    for (auto& [id, obj] : m_objectives) {
        if (obj.controllingTeam == 1) ++team1Objs;
        if (obj.controllingTeam == 2) ++team2Objs;
    }
    if (team1Objs > team2Objs && m_scores.count(1))
        m_scores[1].controlTime += dt;
    else if (team2Objs > team1Objs && m_scores.count(2))
        m_scores[2].controlTime += dt;
}

void BattleSystem::updateObjectives(f32 dt) {
    for (auto& [id, obj] : m_objectives) {
        // Count units near the objective.
        u32 team1Near = 0, team2Near = 0;

        if (m_world) {
            m_world->each<TransformComponent, HealthComponent>(
                [&](EntityID /*eid*/, TransformComponent& tf, HealthComponent& hp) {
                    if (hp.isDead()) return;
                    f32 dx = tf.x - obj.position.x;
                    f32 dz = tf.z - obj.position.z;
                    f32 dist = std::sqrt(dx*dx + dz*dz);
                    if (dist < obj.captureRadius) {
                        // TODO: look up team from AIComponent.teamID or player team.
                        ++team1Near; // placeholder: all entities counted as team1
                    }
                });
        }

        obj.update(dt, 1, team2Near, team1Near);
    }
}

void BattleSystem::updateReinforcements(f32 dt) {
    for (auto& wave : m_waves) {
        wave.cooldownRemaining = std::max(0.f, wave.cooldownRemaining - dt);
        if (!wave.isReady() || !m_ai) continue;

        // Spawn new squads for this wave.
        u32 sqid = m_ai->createSquad(
            "ReinforceSquad_T" + std::to_string(wave.teamID),
            wave.teamID);

        for (u32 s = 0; s < wave.squadCount; ++s) {
            for (u32 m = 0; m < wave.soldiersPerSquad; ++m) {
                // Scatter spawn within the radius.
                f32 angle = static_cast<f32>(s * wave.soldiersPerSquad + m)
                          * 0.618f * 6.283f;
                f32 r = wave.spawnRadius * 0.5f;
                Vec3 spawnPos = {
                    wave.spawnZone.x + std::cos(angle) * r,
                    0.f,
                    wave.spawnZone.z + std::sin(angle) * r
                };
                EntityID aid = m_ai->spawnAgent(*m_world, spawnPos, wave.teamID);
                m_ai->assignToSquad(aid, sqid);
            }
        }

        m_ai->issueSquadOrder(sqid, Tactic::Attack,
                              wave.spawnZone); // move toward battle

        wave.cooldownRemaining = wave.cooldownSeconds; // reset cooldown
        std::cout << "[BattleSystem] Reinforcement wave for team "
                  << wave.teamID << " spawned!\n";
    }
}

void BattleSystem::updateFrontLines() {
    // Gather positions of all AI agents by team.
    // TODO: use a proper spatial query rather than iterating all entities.
    std::unordered_map<u32, std::vector<Vec3>> posByTeam;

    if (m_ai) {
        // We'd normally iterate m_ai->agents(), but the prototype keeps that private.
        // Stub: build empty front lines for now.
        (void)posByTeam;
    }

    for (auto& [teamID, fl] : m_frontLines) {
        auto it = posByTeam.find(teamID);
        if (it != posByTeam.end()) fl.rebuild(it->second);
    }
}

void BattleSystem::checkWinCondition() {
    // Win condition: one team holds all objectives.
    u32 totalObjs = static_cast<u32>(m_objectives.size());
    if (totalObjs == 0) return;

    for (auto& [tid, score] : m_scores) {
        u32 held = 0;
        for (auto& [id, obj] : m_objectives) {
            if (obj.controllingTeam == tid) ++held;
        }
        if (held == totalObjs) {
            m_battleOver  = true;
            m_winningTeam = tid;
            std::cout << "[BattleSystem] Team " << tid << " wins the battle!\n";
            return;
        }
    }
}

u32 BattleSystem::addObjective(BattleObjective obj) {
    obj.objectiveID = m_nextObjID++;
    u32 id = obj.objectiveID;
    m_objectives[id] = std::move(obj);
    return id;
}

void BattleSystem::captureObjective(u32 objID, u32 teamID) {
    auto it = m_objectives.find(objID);
    if (it == m_objectives.end()) return;
    it->second.controllingTeam = teamID;
    it->second.captureProgress = (teamID == 1) ? 1.f : -1.f;
    if (m_scores.count(teamID)) ++m_scores[teamID].objectivesCaptured;
    if (m_onCapture) m_onCapture(objID, teamID);
    std::cout << "[BattleSystem] Objective " << it->second.name
              << " captured by team " << teamID << "\n";
}

BattleObjective* BattleSystem::getObjective(u32 id) {
    auto it = m_objectives.find(id);
    return it != m_objectives.end() ? &it->second : nullptr;
}

void BattleSystem::addReinforcementWave(ReinforcementWave wave) {
    m_waves.push_back(std::move(wave));
}

void BattleSystem::triggerWave(u32 index) {
    if (index < m_waves.size()) m_waves[index].cooldownRemaining = 0.f;
}

const BattleScore& BattleSystem::getScore(u32 teamID) const {
    static BattleScore empty{};
    auto it = m_scores.find(teamID);
    return it != m_scores.end() ? it->second : empty;
}

void BattleSystem::reportKill(u32 killerTeam, u32 victimTeam) {
    if (m_scores.count(killerTeam)) ++m_scores[killerTeam].kills;
    if (m_scores.count(victimTeam)) ++m_scores[victimTeam].deaths;
}

const FrontLine& BattleSystem::getFrontLine(u32 teamID) const {
    static FrontLine empty{};
    auto it = m_frontLines.find(teamID);
    return it != m_frontLines.end() ? it->second : empty;
}

bool BattleSystem::isBattleOver() const { return m_battleOver; }

} // namespace shooter
