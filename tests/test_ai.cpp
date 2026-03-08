#include "ai/BehaviorTree.h"
#include "ai/AIController.h"
#include "ai/SquadManager.h"
#include "ai/PerceptionSystem.h"
#include "ai/CoverSystem.h"
#include <cstdio>
#include <cmath>

using namespace shooter;

extern int g_pass;
extern int g_fail;
extern const char* g_suiteName;

#define ASSERT(expr) \
    do { \
        if (!(expr)) { \
            std::fprintf(stderr, "  FAIL [%s] %s:%d  (%s)\n", \
                         g_suiteName, __FILE__, __LINE__, #expr); \
            ++g_fail; \
        } else { \
            ++g_pass; \
        } \
    } while(0)

void test_ai() {
    g_suiteName = "AI";
    std::printf("\n--- AI ---\n");

    // Behaviour tree: idle when no enemy.
    {
        Blackboard bb;
        bb.hasEnemy = false;
        bb.targetX = 10.0f; bb.targetZ = 10.0f;
        bb.selfX   = 0.0f;  bb.selfZ   = 0.0f;

        auto tree = buildInfantryBT();
        BtStatus status = tree->tick(bb, 0.016f);
        // No enemy → should patrol/idle, returning Success or Running
        ASSERT(status == BtStatus::Success || status == BtStatus::Running);
    }

    // Behaviour tree: combat branch activates when enemy is present.
    {
        Blackboard bb;
        bb.hasEnemy    = true;
        bb.threatDist  = 100.0f;

        auto tree = buildInfantryBT();
        BtStatus status = tree->tick(bb, 0.016f);
        // Enemy present, in range → Running (shooting)
        ASSERT(status != BtStatus::Failure);
    }

    // AIController ticks without crashing.
    {
        AIController ctrl(42, 0);
        ctrl.blackboard().hasEnemy = false;
        ctrl.update(0.016f);
        ctrl.update(0.016f);
        ASSERT(ctrl.entityId() == 42);
        ASSERT(ctrl.factionId() == 0);
    }

    // AIControllerPool manages multiple controllers.
    {
        AIControllerPool pool;
        u32 id0 = pool.create(0, 0);
        u32 id1 = pool.create(1, 1);
        ASSERT(pool.count() == 2);
        ASSERT(pool.get(id0) != nullptr);
        ASSERT(pool.get(id1) != nullptr);
        ASSERT(pool.get(id0)->factionId() == 0);
        ASSERT(pool.get(id1)->factionId() == 1);
        pool.updateAll(0.016f);
        pool.destroy(id0);
        ASSERT(pool.get(id0) == nullptr);
    }

    // Squad assigns formation positions to members.
    {
        AIControllerPool pool;
        SquadManager     mgr;
        u32 sid = mgr.createSquad(0);
        Squad* squad = mgr.getSquad(sid);
        ASSERT(squad != nullptr);

        u32 c0 = pool.create(10, 0);
        u32 c1 = pool.create(11, 0);
        squad->addMember(c0);
        squad->addMember(c1);
        squad->setObjective(500.0f, 0.0f, 500.0f);
        mgr.updateAll(pool, 0.016f);

        // Members should have received target positions near the objective.
        auto* a0 = pool.get(c0);
        ASSERT(a0 != nullptr);
        float dx = a0->blackboard().targetX - 500.0f;
        float dz = a0->blackboard().targetZ - 500.0f;
        float dist = std::sqrt(dx * dx + dz * dz);
        ASSERT(dist < 20.0f); // Within formation radius
    }

    // PerceptionSystem broadcasts and decays.
    {
        PerceptionSystem ps;
        u32 a = ps.registerAgent(1);
        u32 b = ps.registerAgent(2);
        (void)a; (void)b;

        PerceptionEvent ev;
        ev.type            = StimulusType::Visual;
        ev.sourceEntityId  = 2;
        ev.px = 10; ev.py = 0; ev.pz = 0;
        ps.broadcastStimulus(ev, 100.0f, 0.0f);

        const PerceptionState* st = ps.getState(1);
        ASSERT(st && st->canSeeEnemy);

        // After 11 seconds the contact should decay.
        ps.updateSight(11.0f);
        ASSERT(st && !st->canSeeEnemy);
    }

    // CoverSystem finds best cover.
    {
        CoverSystem cs;
        CoverPoint cp;
        cp.x = 10.0f; cp.z = 10.0f;
        cp.normalX = 0.0f; cp.normalZ = 1.0f;
        cp.quality = 1.0f;
        cs.addCoverPoint(cp);
        ASSERT(cs.coverPointCount() == 1);

        // Find cover near origin, threat from north.
        CoverPoint* found = cs.findBestCover(0, 0, 0, -100, 50.0f);
        ASSERT(found != nullptr);

        bool ok = cs.occupy(found, 99);
        ASSERT(ok);
        ASSERT(found->occupied);

        // Cannot occupy twice.
        bool ok2 = cs.occupy(found, 100);
        ASSERT(!ok2);

        cs.release(99);
        ASSERT(!found->occupied);
    }

    std::printf("  AI tests complete\n");
}
