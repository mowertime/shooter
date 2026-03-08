#include "ai/BehaviorTree.h"
#include <cmath>

namespace shooter {

std::unique_ptr<BtNode> buildInfantryBT() {
    // Root selector: try combat, then patrol, then idle.
    auto root = std::make_unique<BtSelector>("Root");

    // ── Combat branch ─────────────────────────────────────────────────────────
    auto combatSeq = std::make_unique<BtSequence>("CombatSequence");
    combatSeq->addChild(std::make_unique<BtCondition>(
        "HasEnemy", [](const Blackboard& bb) { return bb.hasEnemy; }));

    // Take cover if not already in it.
    auto takeCoverSeq = std::make_unique<BtSequence>("TakeCover");
    takeCoverSeq->addChild(std::make_unique<BtCondition>(
        "NotInCover", [](const Blackboard& bb) { return !bb.inCover; }));
    takeCoverSeq->addChild(std::make_unique<BtAction>(
        "MoveToCover", [](Blackboard& bb, float) -> BtStatus {
            // In a real implementation this queries the CoverSystem.
            bb.inCover = true;
            return BtStatus::Success;
        }));

    // Engage enemy.
    auto engageSeq = std::make_unique<BtSequence>("Engage");
    engageSeq->addChild(std::make_unique<BtCondition>(
        "InRange", [](const Blackboard& bb) {
            return bb.threatDist < 200.0f;
        }));
    engageSeq->addChild(std::make_unique<BtAction>(
        "Shoot", [](Blackboard& bb, float) -> BtStatus {
            (void)bb;
            // Fire weapon logic happens in WeaponSystem.
            return BtStatus::Running;
        }));

    combatSeq->addChild(std::move(takeCoverSeq));
    combatSeq->addChild(std::move(engageSeq));
    root->addChild(std::move(combatSeq));

    // ── Patrol branch ─────────────────────────────────────────────────────────
    auto patrol = std::make_unique<BtAction>(
        "Patrol", [](Blackboard& bb, float dt) -> BtStatus {
            // Simple waypoint patrol – walk toward targetX/Z.
            float dx = bb.targetX - bb.selfX;
            float dz = bb.targetZ - bb.selfZ;
            float dist = std::sqrt(dx * dx + dz * dz);
            if (dist < 2.0f) return BtStatus::Success; // reached waypoint
            float speed = 3.0f; // m/s
            bb.selfX += (dx / dist) * speed * dt;
            bb.selfZ += (dz / dist) * speed * dt;
            return BtStatus::Running;
        });
    root->addChild(std::move(patrol));

    // ── Idle ──────────────────────────────────────────────────────────────────
    root->addChild(std::make_unique<BtAction>(
        "Idle", [](Blackboard&, float) -> BtStatus {
            return BtStatus::Success;
        }));

    return root;
}

} // namespace shooter
