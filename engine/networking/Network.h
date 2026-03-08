#pragma once

// ---------------------------------------------------------------------------
// Network.h — Networking: packet serialisation, entity replication, and
// client-side prediction with rollback.
// ---------------------------------------------------------------------------

#include <vector>
#include <unordered_map>
#include <queue>
#include <memory>
#include <functional>
#include <cstring>
#include <unordered_set>

#include "engine/platform/Platform.h"
#include "engine/ecs/ECS.h"
#include "engine/physics/Physics.h"  // Vec3

namespace shooter {

// ---------------------------------------------------------------------------
// NetworkRole
// ---------------------------------------------------------------------------
enum class NetworkRole { Server, Client, ListenServer, Standalone };

// ---------------------------------------------------------------------------
// Packet — raw datagram with reliability metadata.
// ---------------------------------------------------------------------------
struct Packet {
    u16  sequence{0};       ///< monotonically increasing per-connection counter
    u16  ack{0};            ///< last received sequence from the remote
    u32  ackBits{0};        ///< bitmask of the 32 packets before ack
    u8   type{0};           ///< application-level packet type ID
    std::vector<u8> data;   ///< payload (variable-length)
};

// Packet type IDs.
enum PacketType : u8 {
    PKT_ENTITY_STATE    = 1,
    PKT_PLAYER_INPUT    = 2,
    PKT_SNAPSHOT        = 3,
    PKT_RPC             = 4,
    PKT_CONNECT         = 5,
    PKT_DISCONNECT      = 6,
};

// ---------------------------------------------------------------------------
// EntitySnapshot — serialisable state of one entity at a given tick.
// ---------------------------------------------------------------------------
struct EntitySnapshot {
    EntityID entityID{INVALID_ENTITY};
    u32      tick{0};
    Vec3     position{};
    Vec3     velocity{};
    f32      health{0};
    u8       flags{0};       ///< bitmask: alive, visible, etc.
};

// ---------------------------------------------------------------------------
// PlayerInput — sent from client → server every frame.
// ---------------------------------------------------------------------------
struct NetworkPlayerInput {
    u32  tick{0};
    f32  moveX{0};     ///< [-1, 1] strafe
    f32  moveZ{0};     ///< [-1, 1] forward
    f32  yaw{0};       ///< radians
    f32  pitch{0};
    bool jump{false};
    bool fire{false};
    bool aim{false};
    bool crouch{false};
};

// ---------------------------------------------------------------------------
// EntityReplication — tracks which entities need to be synced to clients.
// ---------------------------------------------------------------------------
class EntityReplication {
public:
    void markDirty(EntityID id) { m_dirtySet.insert(id); }
    void clearDirty()           { m_dirtySet.clear(); }

    /// Serialize all dirty entities into a snapshot packet payload.
    std::vector<u8> buildSnapshotPayload(ECSWorld& world,
                                         u32 tick) const;

    /// Apply a received snapshot payload to the local ECS world.
    void applySnapshot(ECSWorld& world,
                       const std::vector<u8>& payload);

private:
    std::unordered_set<EntityID> m_dirtySet;

    // Simple serialisation helpers.
    static void writeU64(std::vector<u8>& buf, u64 v);
    static void writeF32(std::vector<u8>& buf, f32 v);
    static u64  readU64(const u8*& ptr);
    static f32  readF32(const u8*& ptr);
};

// ---------------------------------------------------------------------------
// PredictionSystem — client-side prediction + server reconciliation (rollback).
//
// The client applies inputs immediately (predict) and keeps a history.
// When the server authoritative state arrives, it is compared with the
// predicted state; on mismatch the client re-simulates (rollback).
// ---------------------------------------------------------------------------
class PredictionSystem {
public:
    static constexpr u32 INPUT_HISTORY_SIZE = 128;

    void recordInput(const NetworkPlayerInput& input);
    void applyPrediction(ECSWorld& world, EntityID playerEntity,
                         const NetworkPlayerInput& input, f32 dt);

    /// Called when the server snapshot arrives.  Triggers rollback if needed.
    void reconcile(ECSWorld& world, EntityID playerEntity,
                   const EntitySnapshot& serverState);

private:
    struct HistoryEntry { NetworkPlayerInput input; EntitySnapshot predictedState; };

    std::array<HistoryEntry, INPUT_HISTORY_SIZE> m_history;
    u32  m_historyHead{0};
    u32  m_lastAckedTick{0};
};

// ===========================================================================
// NetworkManager
// ===========================================================================
class NetworkManager {
public:
    NetworkManager()  = default;
    ~NetworkManager() = default;

    bool initialize(NetworkRole role, u16 port = 7777);
    void shutdown();

    /// Call every frame to send/receive packets.
    void update(ECSWorld& world, f32 dt);

    // ---- Server API --------------------------------------------------------
    bool listenForClients();
    void broadcastSnapshot(ECSWorld& world, u32 tick);

    // ---- Client API --------------------------------------------------------
    bool connectToServer(const std::string& address, u16 port);
    void sendPlayerInput(const NetworkPlayerInput& input);

    [[nodiscard]] NetworkRole role()        const { return m_role; }
    [[nodiscard]] bool        isConnected() const { return m_connected; }

    EntityReplication& replication()  { return m_replication; }
    PredictionSystem&  prediction()   { return m_prediction; }

private:
    NetworkRole       m_role{NetworkRole::Standalone};
    bool              m_connected{false};
    u16               m_port{7777};
    u32               m_currentTick{0};

    EntityReplication m_replication;
    PredictionSystem  m_prediction;

    // TODO: platform socket handle (SOCKET on Windows, int fd on POSIX)
    int m_socket{-1};

    std::queue<Packet> m_sendQueue;
    std::queue<Packet> m_recvQueue;
};

} // namespace shooter
