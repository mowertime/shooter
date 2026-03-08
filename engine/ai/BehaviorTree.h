#pragma once

// ---------------------------------------------------------------------------
// BehaviorTree.h — Composable behavior tree for AI decision making.
//
// Nodes return one of three statuses each tick:
//   Success  — the node's goal was achieved
//   Failure  — the node's goal could not be achieved
//   Running  — the node is still working (asynchronous)
//
// Three composite node types:
//   Sequence — children run left-to-right; fails on first child Failure
//   Selector — children run left-to-right; succeeds on first child Success
//   Parallel — all children run simultaneously; configurable success threshold
// ---------------------------------------------------------------------------

#include <vector>
#include <memory>
#include <functional>
#include <string>

#include "engine/platform/Platform.h"
#include "engine/ecs/ECS.h"

namespace shooter {

// ---------------------------------------------------------------------------
// NodeStatus — result of a single BehaviorNode::tick() call.
// ---------------------------------------------------------------------------
enum class NodeStatus { Success, Failure, Running };

// ---------------------------------------------------------------------------
// BehaviorNode — abstract base.
// ---------------------------------------------------------------------------
class BehaviorNode {
public:
    explicit BehaviorNode(std::string name) : m_name(std::move(name)) {}
    virtual ~BehaviorNode() = default;

    virtual NodeStatus tick(EntityID agent, ECSWorld& world, f32 dt) = 0;

    /// Called when the node transitions from Running → anything else.
    virtual void onAbort(EntityID /*agent*/) {}

    [[nodiscard]] const std::string& nodeName() const { return m_name; }

protected:
    std::string m_name;
};

// ---------------------------------------------------------------------------
// Composite nodes
// ---------------------------------------------------------------------------

/// Sequence ("AND"): runs children in order; fails if any child fails.
class SequenceNode : public BehaviorNode {
public:
    explicit SequenceNode(std::string name) : BehaviorNode(std::move(name)) {}

    void addChild(std::unique_ptr<BehaviorNode> child) {
        m_children.push_back(std::move(child));
    }

    NodeStatus tick(EntityID agent, ECSWorld& world, f32 dt) override {
        for (auto& child : m_children) {
            NodeStatus s = child->tick(agent, world, dt);
            if (s != NodeStatus::Success) return s; // Failure or Running
        }
        return NodeStatus::Success;
    }

private:
    std::vector<std::unique_ptr<BehaviorNode>> m_children;
};

/// Selector ("OR"): runs children in order; succeeds if any child succeeds.
class SelectorNode : public BehaviorNode {
public:
    explicit SelectorNode(std::string name) : BehaviorNode(std::move(name)) {}

    void addChild(std::unique_ptr<BehaviorNode> child) {
        m_children.push_back(std::move(child));
    }

    NodeStatus tick(EntityID agent, ECSWorld& world, f32 dt) override {
        for (auto& child : m_children) {
            NodeStatus s = child->tick(agent, world, dt);
            if (s != NodeStatus::Failure) return s;
        }
        return NodeStatus::Failure;
    }

private:
    std::vector<std::unique_ptr<BehaviorNode>> m_children;
};

/// Parallel: all children tick; succeeds when >= successThreshold children succeed.
class ParallelNode : public BehaviorNode {
public:
    explicit ParallelNode(std::string name, u32 successThreshold = 1)
        : BehaviorNode(std::move(name))
        , m_successThreshold(successThreshold) {}

    void addChild(std::unique_ptr<BehaviorNode> child) {
        m_children.push_back(std::move(child));
    }

    NodeStatus tick(EntityID agent, ECSWorld& world, f32 dt) override {
        u32 successes = 0;
        u32 failures  = 0;
        for (auto& child : m_children) {
            NodeStatus s = child->tick(agent, world, dt);
            if (s == NodeStatus::Success) ++successes;
            if (s == NodeStatus::Failure) ++failures;
        }
        if (successes >= m_successThreshold) return NodeStatus::Success;
        if (failures > static_cast<u32>(m_children.size()) - m_successThreshold)
            return NodeStatus::Failure;
        return NodeStatus::Running;
    }

private:
    std::vector<std::unique_ptr<BehaviorNode>> m_children;
    u32 m_successThreshold{1};
};

// ---------------------------------------------------------------------------
// Leaf nodes
// ---------------------------------------------------------------------------

/// Action — wraps a callable that performs one unit of work.
class ActionNode : public BehaviorNode {
public:
    using ActionFn = std::function<NodeStatus(EntityID, ECSWorld&, f32)>;

    ActionNode(std::string name, ActionFn fn)
        : BehaviorNode(std::move(name)), m_fn(std::move(fn)) {}

    NodeStatus tick(EntityID agent, ECSWorld& world, f32 dt) override {
        return m_fn(agent, world, dt);
    }

private:
    ActionFn m_fn;
};

/// Condition — returns Success/Failure based on a predicate; never Running.
class ConditionNode : public BehaviorNode {
public:
    using ConditionFn = std::function<bool(EntityID, ECSWorld&)>;

    ConditionNode(std::string name, ConditionFn fn)
        : BehaviorNode(std::move(name)), m_fn(std::move(fn)) {}

    NodeStatus tick(EntityID agent, ECSWorld& world, f32 /*dt*/) override {
        return m_fn(agent, world) ? NodeStatus::Success : NodeStatus::Failure;
    }

private:
    ConditionFn m_fn;
};

// ---------------------------------------------------------------------------
// BehaviorTree — owns a root node and ticks it each update.
// ---------------------------------------------------------------------------
class BehaviorTree {
public:
    void setRoot(std::unique_ptr<BehaviorNode> root) {
        m_root = std::move(root);
    }

    NodeStatus tick(EntityID agent, ECSWorld& world, f32 dt) {
        if (!m_root) return NodeStatus::Failure;
        return m_root->tick(agent, world, dt);
    }

private:
    std::unique_ptr<BehaviorNode> m_root;
};

} // namespace shooter
