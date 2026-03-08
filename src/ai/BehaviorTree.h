#pragma once

#include "core/Platform.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace shooter {

/// Result of evaluating a behaviour tree node.
enum class BtStatus : u8 {
    Running = 0, ///< Node is still executing
    Success = 1,
    Failure = 2,
};

/// Blackboard: shared key-value state for a single AI agent.
struct Blackboard {
    float targetX{0}, targetY{0}, targetZ{0};  ///< Current move/attack target
    float selfX{0},   selfY{0},   selfZ{0};    ///< Agent position cache
    float threatDist{9999.0f};                 ///< Distance to nearest enemy
    u32   targetEntityId{UINT32_MAX};
    u32   squadId{UINT32_MAX};
    bool  hasEnemy{false};
    bool  inCover{false};
    bool  isAlerted{false};
};

/// Abstract behaviour tree node.
class BtNode {
public:
    virtual ~BtNode() = default;
    virtual BtStatus tick(Blackboard& bb, float dt) = 0;
    virtual const char* name() const = 0;
};

/// Selector: returns Success on first child success (OR).
class BtSelector final : public BtNode {
public:
    explicit BtSelector(std::string n) : m_name(std::move(n)) {}
    void addChild(std::unique_ptr<BtNode> child) {
        m_children.push_back(std::move(child));
    }
    BtStatus tick(Blackboard& bb, float dt) override {
        for (auto& c : m_children) {
            if (c->tick(bb, dt) == BtStatus::Success) return BtStatus::Success;
        }
        return BtStatus::Failure;
    }
    const char* name() const override { return m_name.c_str(); }

private:
    std::string                          m_name;
    std::vector<std::unique_ptr<BtNode>> m_children;
};

/// Sequence: returns Failure on first child failure (AND).
class BtSequence final : public BtNode {
public:
    explicit BtSequence(std::string n) : m_name(std::move(n)) {}
    void addChild(std::unique_ptr<BtNode> child) {
        m_children.push_back(std::move(child));
    }
    BtStatus tick(Blackboard& bb, float dt) override {
        for (auto& c : m_children) {
            BtStatus s = c->tick(bb, dt);
            if (s != BtStatus::Success) return s;
        }
        return BtStatus::Success;
    }
    const char* name() const override { return m_name.c_str(); }

private:
    std::string                          m_name;
    std::vector<std::unique_ptr<BtNode>> m_children;
};

/// Action node backed by a lambda.
class BtAction final : public BtNode {
public:
    using ActionFunc = std::function<BtStatus(Blackboard&, float dt)>;

    BtAction(std::string n, ActionFunc fn)
        : m_name(std::move(n)), m_fn(std::move(fn)) {}

    BtStatus tick(Blackboard& bb, float dt) override {
        return m_fn ? m_fn(bb, dt) : BtStatus::Failure;
    }
    const char* name() const override { return m_name.c_str(); }

private:
    std::string m_name;
    ActionFunc  m_fn;
};

/// Condition node backed by a predicate.
class BtCondition final : public BtNode {
public:
    using CondFunc = std::function<bool(const Blackboard&)>;

    BtCondition(std::string n, CondFunc fn)
        : m_name(std::move(n)), m_fn(std::move(fn)) {}

    BtStatus tick(Blackboard& bb, float) override {
        return (m_fn && m_fn(bb)) ? BtStatus::Success : BtStatus::Failure;
    }
    const char* name() const override { return m_name.c_str(); }

private:
    std::string m_name;
    CondFunc    m_fn;
};

/// Convenience factory that builds a basic infantry combat behaviour tree.
std::unique_ptr<BtNode> buildInfantryBT();

} // namespace shooter
