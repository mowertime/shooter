// ---------------------------------------------------------------------------
// Network.cpp — NetworkManager, replication, and prediction implementation.
// ---------------------------------------------------------------------------

#include "engine/networking/Network.h"
#include "engine/ecs/ECS.h"

#include <iostream>
#include <cstring>
#include <algorithm>

namespace shooter {

// ===========================================================================
// EntityReplication
// ===========================================================================

void EntityReplication::writeU64(std::vector<u8>& buf, u64 v) {
    for (int i = 0; i < 8; ++i)
        buf.push_back(static_cast<u8>((v >> (i * 8)) & 0xFF));
}

void EntityReplication::writeF32(std::vector<u8>& buf, f32 v) {
    u32 bits;
    std::memcpy(&bits, &v, 4);
    for (int i = 0; i < 4; ++i)
        buf.push_back(static_cast<u8>((bits >> (i * 8)) & 0xFF));
}

u64 EntityReplication::readU64(const u8*& ptr) {
    u64 v = 0;
    for (int i = 0; i < 8; ++i) v |= static_cast<u64>(*ptr++) << (i * 8);
    return v;
}

f32 EntityReplication::readF32(const u8*& ptr) {
    u32 bits = 0;
    for (int i = 0; i < 4; ++i) bits |= static_cast<u32>(*ptr++) << (i * 8);
    f32 v;
    std::memcpy(&v, &bits, 4);
    return v;
}

std::vector<u8> EntityReplication::buildSnapshotPayload(ECSWorld& world, u32 tick) const {
    std::vector<u8> buf;
    // Header: tick (4 bytes) + entity count (4 bytes)
    for (int i = 0; i < 4; ++i) buf.push_back(static_cast<u8>((tick >> (i*8)) & 0xFF));
    u32 countPos = static_cast<u32>(buf.size());
    buf.push_back(0); buf.push_back(0); buf.push_back(0); buf.push_back(0);

    u32 count = 0;
    for (EntityID id : m_dirtySet) {
        auto* tf = world.getComponent<TransformComponent>(id);
        auto* vl = world.getComponent<VelocityComponent>(id);
        auto* hp = world.getComponent<HealthComponent>(id);
        if (!tf) continue;

        writeU64(buf, id);
        writeF32(buf, tf->x); writeF32(buf, tf->y); writeF32(buf, tf->z);
        writeF32(buf, vl ? vl->vx : 0.f);
        writeF32(buf, vl ? vl->vy : 0.f);
        writeF32(buf, vl ? vl->vz : 0.f);
        writeF32(buf, hp ? hp->current : 0.f);
        ++count;
    }

    // Patch count
    for (int i = 0; i < 4; ++i)
        buf[countPos + i] = static_cast<u8>((count >> (i*8)) & 0xFF);

    return buf;
}

void EntityReplication::applySnapshot(ECSWorld& world,
                                       const std::vector<u8>& payload) {
    if (payload.size() < 8) return;
    const u8* ptr = payload.data();

    u32 tick = 0;
    for (int i = 0; i < 4; ++i) tick |= static_cast<u32>(*ptr++) << (i*8);

    u32 count = 0;
    for (int i = 0; i < 4; ++i) count |= static_cast<u32>(*ptr++) << (i*8);

    for (u32 e = 0; e < count; ++e) {
        EntityID id = readU64(ptr);
        f32 px = readF32(ptr), py = readF32(ptr), pz = readF32(ptr);
        f32 vx = readF32(ptr), vy = readF32(ptr), vz = readF32(ptr);
        f32 hp = readF32(ptr);

        if (!world.isValid(id)) continue;

        if (auto* tf = world.getComponent<TransformComponent>(id)) {
            tf->x = px; tf->y = py; tf->z = pz;
        }
        if (auto* vl = world.getComponent<VelocityComponent>(id)) {
            vl->vx = vx; vl->vy = vy; vl->vz = vz;
        }
        if (auto* h = world.getComponent<HealthComponent>(id)) {
            h->current = hp;
        }
    }
    (void)tick; // used for ordering / jitter buffer in production
}

// ===========================================================================
// PredictionSystem
// ===========================================================================

void PredictionSystem::recordInput(const NetworkPlayerInput& input) {
    m_history[m_historyHead % INPUT_HISTORY_SIZE].input = input;
    ++m_historyHead;
}

void PredictionSystem::applyPrediction(ECSWorld& world,
                                        EntityID playerEntity,
                                        const NetworkPlayerInput& input,
                                        f32 dt) {
    auto* tf = world.getComponent<TransformComponent>(playerEntity);
    auto* vl = world.getComponent<VelocityComponent>(playerEntity);
    if (!tf || !vl) return;

    // Simple predicted movement.
    const f32 speed = 5.f; // m/s
    vl->vx = input.moveX * speed;
    vl->vz = input.moveZ * speed;
    tf->x += vl->vx * dt;
    tf->z += vl->vz * dt;
}

void PredictionSystem::reconcile(ECSWorld& world,
                                  EntityID playerEntity,
                                  const EntitySnapshot& serverState) {
    auto* tf = world.getComponent<TransformComponent>(playerEntity);
    if (!tf) return;

    // Compare server position with our predicted position.
    Vec3 serverPos = serverState.position;
    f32 errX = tf->x - serverPos.x;
    f32 errZ = tf->z - serverPos.z;
    f32 errSq = errX*errX + errZ*errZ;

    if (errSq < 0.01f) return; // Close enough — no rollback needed.

    // Rollback: snap to server state then re-simulate from last acked tick.
    tf->x = serverPos.x;
    tf->y = serverPos.y;
    tf->z = serverPos.z;

    // Re-apply all inputs after serverState.tick.
    for (u32 i = m_lastAckedTick; i < m_historyHead; ++i) {
        const auto& entry = m_history[i % INPUT_HISTORY_SIZE];
        if (entry.input.tick <= serverState.tick) continue;
        applyPrediction(world, playerEntity, entry.input, 1.f / 60.f);
    }
    m_lastAckedTick = serverState.tick;
}

// ===========================================================================
// NetworkManager
// ===========================================================================

bool NetworkManager::initialize(NetworkRole role, u16 port) {
    m_role = role;
    m_port = port;
    // TODO: platform socket initialisation (WSAStartup on Windows, etc.)
    std::cout << "[Network] Initialized as "
              << (role == NetworkRole::Server ? "Server" :
                  role == NetworkRole::Client ? "Client" : "Standalone")
              << " on port " << port << " (stub)\n";
    return true;
}

void NetworkManager::shutdown() {
    // TODO: close socket, WSACleanup on Windows
    std::cout << "[Network] Shutdown\n";
    m_connected = false;
}

void NetworkManager::update(ECSWorld& world, f32 /*dt*/) {
    ++m_currentTick;

    // TODO: recv all pending datagrams → m_recvQueue
    // TODO: process m_recvQueue (deserialise packets, dispatch by type)

    if (m_role == NetworkRole::Server) {
        // Every 2 ticks (~33 ms at 60 Hz) send a snapshot to all clients.
        if (m_currentTick % 2 == 0) {
            broadcastSnapshot(world, m_currentTick);
        }
    }

    // TODO: flush m_sendQueue → actual socket sends
}

bool NetworkManager::listenForClients() {
    // TODO: bind socket to m_port, set non-blocking
    std::cout << "[Network] Listening on port " << m_port << " (stub)\n";
    return true;
}

void NetworkManager::broadcastSnapshot(ECSWorld& world, u32 tick) {
    auto payload = m_replication.buildSnapshotPayload(world, tick);
    Packet pkt;
    pkt.sequence = static_cast<u16>(m_currentTick);
    pkt.type     = PKT_SNAPSHOT;
    pkt.data     = std::move(payload);
    m_sendQueue.push(std::move(pkt));
    // TODO: iterate connected clients and send pkt via UDP socket
}

bool NetworkManager::connectToServer(const std::string& address, u16 port) {
    // TODO: resolve address, connect UDP socket
    std::cout << "[Network] Connecting to " << address << ":" << port << " (stub)\n";
    m_connected = true;
    return true;
}

void NetworkManager::sendPlayerInput(const NetworkPlayerInput& input) {
    // Serialise and enqueue; sent in update().
    Packet pkt;
    pkt.type = PKT_PLAYER_INPUT;
    // Pack input into bytes (simplified).
    auto& d = pkt.data;
    for (int i = 0; i < 4; ++i) d.push_back(static_cast<u8>((input.tick >> (i*8)) & 0xFF));
    m_prediction.recordInput(input);
    m_sendQueue.push(std::move(pkt));
}

} // namespace shooter
