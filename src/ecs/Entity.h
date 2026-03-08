#pragma once

#include "core/Platform.h"
#include <array>
#include <bitset>
#include <cstdint>
#include <limits>

namespace shooter {

// ── Entity ────────────────────────────────────────────────────────────────────

/// Maximum number of distinct component types in the engine.
static constexpr u32 MAX_COMPONENTS = 128;

/// Maximum number of live entities.
static constexpr u32 MAX_ENTITIES   = 1'000'000;

/// An entity is a versioned 64-bit handle: upper 32 bits = version,
/// lower 32 bits = index.  The versioning prevents dangling handles
/// after entity recycling.
struct Entity {
    static constexpr u64 INVALID = std::numeric_limits<u64>::max();

    u64 id{INVALID};

    bool isValid() const { return id != INVALID; }
    u32  index()   const { return static_cast<u32>(id & 0xFFFFFFFFu); }
    u32  version() const { return static_cast<u32>(id >> 32); }

    bool operator==(Entity o) const { return id == o.id; }
    bool operator!=(Entity o) const { return id != o.id; }
};

using ComponentMask = std::bitset<MAX_COMPONENTS>;

} // namespace shooter
