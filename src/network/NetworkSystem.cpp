#include "network/NetworkSystem.h"
#include "core/Logger.h"

namespace shooter {

struct NetworkSystem::Impl {
    NetworkDesc   desc;
    bool          connected{false};
    u32           clientCount{0};
    u32           pingMs{0};
    PacketHandler handler;
    float         tickTimer{0};
};

static NetworkSystem s_net;

NetworkSystem& NetworkSystem::get() { return s_net; }

bool NetworkSystem::init(const NetworkDesc& desc) {
    m_impl = std::make_unique<Impl>();
    m_impl->desc = desc;

    switch (desc.role) {
        case NetworkRole::Offline:
            LOG_INFO("NetworkSystem", "Starting in offline mode");
            m_impl->connected = true;
            break;
        case NetworkRole::DedicatedServer:
            LOG_INFO("NetworkSystem", "Dedicated server on port %u (max %u clients)",
                     desc.port, desc.maxClients);
            // Bind UDP socket, start accept loop (stub: no real socket).
            m_impl->connected = true;
            break;
        case NetworkRole::Client:
            LOG_INFO("NetworkSystem", "Connecting to %s:%u",
                     desc.serverAddress.c_str(), desc.port);
            // Initiate UDP handshake (stub).
            m_impl->connected = true;
            break;
        default:
            break;
    }
    return true;
}

void NetworkSystem::shutdown() {
    if (m_impl) {
        LOG_INFO("NetworkSystem", "Shutting down");
        m_impl->connected = false;
    }
}

void NetworkSystem::update(float dt) {
    if (!m_impl) return;
    m_impl->tickTimer += dt;
    float tickInterval = 1.0f / static_cast<float>(m_impl->desc.tickRate);
    if (m_impl->tickTimer >= tickInterval) {
        m_impl->tickTimer -= tickInterval;
        // In a real implementation:
        // Server: snapshot all replicated entities → send to clients.
        // Client: send input command for this tick.
    }
}

bool NetworkSystem::isConnected()          const { return m_impl && m_impl->connected;    }
u32  NetworkSystem::connectedClientCount() const { return m_impl ? m_impl->clientCount : 0; }
u32  NetworkSystem::pingMs()               const { return m_impl ? m_impl->pingMs      : 0; }
const NetworkDesc& NetworkSystem::desc()   const { return m_impl->desc; }

void NetworkSystem::setPacketHandler(PacketHandler h) {
    if (m_impl) m_impl->handler = std::move(h);
}

} // namespace shooter
