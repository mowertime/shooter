#pragma once

#include "core/Platform.h"
#include <string>
#include <vector>
#include <functional>

namespace shooter {

/// Render graph pass type.
enum class PassType : u8 {
    Graphics,
    Compute,
    RayTracing,
    Copy,
};

/// Describes a single node in the frame render graph.
struct RenderPassDesc {
    std::string name;
    PassType    type{PassType::Graphics};
    bool        enabled{true};
};

/// Declarative render graph.
///
/// The graph analyses data dependencies between passes to automatically
/// insert barriers and schedule work across the GPU timeline.
/// Unused passes are culled to avoid unnecessary GPU work.
class RenderGraph {
public:
    using PassCallback = std::function<void()>;

    /// Register a pass.  Returns its pass index.
    u32 addPass(RenderPassDesc desc, PassCallback callback);

    /// Compile the graph (resolve dependencies, insert barriers).
    void compile();

    /// Execute all enabled passes in dependency order.
    void execute();

    void setPassEnabled(u32 passIndex, bool enabled);

private:
    struct Pass {
        RenderPassDesc desc;
        PassCallback   callback;
        std::vector<u32> dependencies;
    };
    std::vector<Pass> m_passes;
    bool m_compiled{false};
};

} // namespace shooter
