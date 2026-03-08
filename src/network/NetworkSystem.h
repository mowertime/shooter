#pragma once

#include "core/Platform.h"
#include <memory>
#include <string>
#include <functional>

namespace shooter {

/// Network role for this instance.
enum class NetworkRole : u8 {
    Offline,
    DedicatedServer,
    ListenServer,
    Client,
};

/// Configuration for the network subsystem.
struct NetworkDesc {
    NetworkRole role{NetworkRole::Offline};
    std::string serverAddress{"127.0.0.1"};
    u16         port{7777};
    u32         maxClients{64};
    u32         tickRate{30}; ///< Network update rate (Hz)
};

/// Network packet types (subset).
enum class PacketType : u8 {
    Connect,
    Disconnect,
    EntitySnapshot,
    InputCommand,
    ChatMessage,
    ServerInfo,
};

/// Abstract network system.
///
/// The server ticks entity state at \p tickRate Hz and sends snapshots
/// to all clients.  Clients send input commands and use dead-reckoning
/// plus rollback to minimise perceived latency.
class NetworkSystem {
public:
    static NetworkSystem& get();

    bool init(const NetworkDesc& desc);
    void shutdown();

    /// Process received packets, update ping measurements.
    void update(float dt);

    /// Returns true when a stable connection is established.
    bool isConnected() const;

    u32 connectedClientCount() const;
    u32 pingMs()               const;

    const NetworkDesc& desc() const;

    using PacketHandler = std::function<void(PacketType, const u8* data, u32 size)>;
    void setPacketHandler(PacketHandler h);

    NetworkSystem() = default;
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace shooter
