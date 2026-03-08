#pragma once

#include "ai/BehaviorTree.h"
#include <memory>
#include <vector>

namespace shooter {

/// Per-agent AI controller.
///
/// Owns a Blackboard and a BehaviorTree root.  Updated by AIUpdateSystem.
class AIController {
public:
    explicit AIController(u32 entityId, u8 factionId);

    /// Tick the behaviour tree.
    void update(float dt);

    Blackboard&       blackboard()       { return m_bb; }
    const Blackboard& blackboard() const { return m_bb; }

    u32 entityId()  const { return m_entityId;  }
    u8  factionId() const { return m_factionId; }

    void setTree(std::unique_ptr<BtNode> root) {
        m_tree = std::move(root);
    }

private:
    u32                      m_entityId;
    u8                       m_factionId;
    Blackboard               m_bb;
    std::unique_ptr<BtNode>  m_tree;
    BtStatus                 m_lastStatus{BtStatus::Running};
};

/// Global AI controller pool.
class AIControllerPool {
public:
    AIControllerPool() = default;

    /// Allocate a new controller.  Returns its index in the pool.
    u32 create(u32 entityId, u8 factionId);

    /// Destroy a controller.
    void destroy(u32 controllerId);

    AIController* get(u32 controllerId);

    /// Update all active controllers.
    void updateAll(float dt);

    u32 count() const;

private:
    std::vector<std::unique_ptr<AIController>> m_controllers;
};

} // namespace shooter
