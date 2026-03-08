// ---------------------------------------------------------------------------
// AI.cpp — AIManager implementation.
// ---------------------------------------------------------------------------

#include "engine/ai/AI.h"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace shooter {

bool AIManager::initialize(u32 maxAgents) {
    m_agents.reserve(maxAgents);
    m_squads.reserve(64);
    std::cout << "[AI] Initialized (max " << maxAgents << " agents)\n";
    return true;
}

void AIManager::shutdown() {
    m_agents.clear();
    m_squads.clear();
    std::cout << "[AI] Shutdown\n";
}

// ---------------------------------------------------------------------------
// Spawn / despawn
// ---------------------------------------------------------------------------

EntityID AIManager::spawnAgent(ECSWorld& world, Vec3 position, u32 teamID) {
    EntityID id = world.createEntity();

    // Set up ECS components.
    TransformComponent tf;
    tf.x = position.x; tf.y = position.y; tf.z = position.z;
    world.addComponent<TransformComponent>(id, tf);
    world.addComponent<VelocityComponent>(id);
    world.addComponent<HealthComponent>(id, {100.f, 100.f, 0.2f});

    AIComponent aiComp;
    aiComp.isActive = true;
    world.addComponent<AIComponent>(id, aiComp);

    // Create the AIAgent record.
    AIAgent agent;
    agent.entityID = id;
    agent.teamID   = teamID;
    buildDefaultBehaviorTree(agent);
    m_agents[id] = std::move(agent);

    return id;
}

void AIManager::despawnAgent(ECSWorld& world, EntityID id) {
    world.destroyEntity(id);
    m_agents.erase(id);
}

void AIManager::assignToSquad(EntityID agentID, u32 squadID) {
    auto agentIt = m_agents.find(agentID);
    auto squadIt = m_squads.find(squadID);
    if (agentIt == m_agents.end() || squadIt == m_squads.end()) return;

    agentIt->second.squadID = squadID;
    squadIt->second.memberIDs.push_back(agentID);
}

// ---------------------------------------------------------------------------
// Squad management
// ---------------------------------------------------------------------------

u32 AIManager::createSquad(std::string name, u32 teamID) {
    u32 sid = m_nextSquadID++;
    Squad sq;
    sq.squadID = sid;
    sq.name    = std::move(name);
    sq.teamID  = teamID;
    m_squads[sid] = std::move(sq);
    return sid;
}

Squad* AIManager::getSquad(u32 squadID) {
    auto it = m_squads.find(squadID);
    return it != m_squads.end() ? &it->second : nullptr;
}

void AIManager::issueSquadOrder(u32 squadID, Tactic tactic, Vec3 objective) {
    Squad* sq = getSquad(squadID);
    if (!sq) return;
    sq->currentTactic     = tactic;
    sq->objectivePosition = objective;
    std::cout << "[AI] Squad " << sq->name
              << " received tactic " << static_cast<int>(tactic) << "\n";
}

// ---------------------------------------------------------------------------
// Per-frame update
// ---------------------------------------------------------------------------

void AIManager::update(ECSWorld& world, f32 dt, Vec3 playerPos) {
    for (auto& [eid, agent] : m_agents) {
        // LOD assignment based on distance to player.
        updateLOD(agent, playerPos);

        if (agent.lod == AILODLevel::Dormant) {
            // Dormant agents only update transform via simple patrol (stub).
            continue;
        }

        // Perception update (reduced-LOD agents update at 10 Hz instead of 60 Hz).
        agent.tickAccumulator += dt;
        f32 tickRate = (agent.lod == AILODLevel::Reduced) ? 0.1f : 0.016f;
        if (agent.tickAccumulator >= tickRate) {
            agent.tickAccumulator = 0.f;
            updatePerception(agent, world);
            agent.behaviorTree.tick(eid, world, tickRate);
        }
    }
}

void AIManager::updateLOD(AIAgent& agent, Vec3 playerPos) {
    // In a real build we'd cache the TransformComponent* directly in AIAgent.
    // For the prototype we compute distance from the stored agent record.
    (void)playerPos;

    f32 dist = agent.distanceToPlayer; // updated below if we had the position

    if (dist < FULL_AI_RADIUS)         agent.lod = AILODLevel::Full;
    else if (dist < REDUCED_AI_RADIUS) agent.lod = AILODLevel::Reduced;
    else                               agent.lod = AILODLevel::Dormant;
}

void AIManager::updatePerception(AIAgent& agent, ECSWorld& world) {
    // Scan all enemy entities for sight/hearing.
    // In production: use a spatial hash / BVH for O(log n) lookups.
    agent.perception.timeSinceSaw += 0.016f;
    agent.perception.canSeeEnemy  = false;
    agent.perception.canHearEnemy = false;

    // Iterate all health components to find potential threats.
    world.each<TransformComponent, HealthComponent>(
        [&](EntityID eid, TransformComponent& tf, HealthComponent& hp) {
            if (eid == agent.entityID) return;
            if (hp.isDead()) return;

            // Find this agent's own transform.
            auto* myTf = world.getComponent<TransformComponent>(agent.entityID);
            if (!myTf) return;

            f32 dx = tf.x - myTf->x;
            f32 dz = tf.z - myTf->z;
            f32 dist = std::sqrt(dx*dx + dz*dz);

            if (dist < agent.perception.sightRange) {
                // TODO: proper FOV cone check + line-of-sight raycast
                agent.perception.canSeeEnemy      = true;
                agent.perception.lastKnownEnemy    = eid;
                agent.perception.lastKnownEnemyPos = {tf.x, tf.y, tf.z};
                agent.perception.timeSinceSaw      = 0.f;
            }
        });
}

void AIManager::buildDefaultBehaviorTree(AIAgent& agent) {
    // Build a simple combat behavior tree:
    //
    //   Selector (root)
    //   ├── Sequence (combat)
    //   │   ├── Condition: can see enemy?
    //   │   └── Action: attack enemy
    //   └── Sequence (patrol)
    //       └── Action: patrol waypoints

    auto root = std::make_unique<SelectorNode>("Root");

    // Combat branch
    auto combatSeq = std::make_unique<SequenceNode>("CombatSequence");
    combatSeq->addChild(std::make_unique<ConditionNode>(
        "CanSeeEnemy",
        [this, eid = agent.entityID](EntityID /*a*/, ECSWorld& /*w*/) -> bool {
            auto it = m_agents.find(eid);
            return it != m_agents.end() && it->second.perception.canSeeEnemy;
        }));
    combatSeq->addChild(std::make_unique<ActionNode>(
        "AttackEnemy",
        [](EntityID /*a*/, ECSWorld& /*w*/, f32 /*dt*/) -> NodeStatus {
            // TODO: aim, fire, take cover
            return NodeStatus::Running;
        }));

    // Patrol branch
    auto patrolSeq = std::make_unique<SequenceNode>("PatrolSequence");
    patrolSeq->addChild(std::make_unique<ActionNode>(
        "PatrolWaypoints",
        [](EntityID /*a*/, ECSWorld& /*w*/, f32 /*dt*/) -> NodeStatus {
            // TODO: move to next waypoint along a patrol path
            return NodeStatus::Running;
        }));

    root->addChild(std::move(combatSeq));
    root->addChild(std::move(patrolSeq));

    agent.behaviorTree.setRoot(std::move(root));
}

} // namespace shooter
