#pragma once

// ---------------------------------------------------------------------------
// Audio.h — 3D spatial audio system with battlefield acoustics.
// ---------------------------------------------------------------------------

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "engine/platform/Platform.h"
#include "engine/physics/Physics.h"  // Vec3

namespace shooter {

// ---------------------------------------------------------------------------
// SoundHandle — opaque identifier for a playing sound instance.
// ---------------------------------------------------------------------------
enum class SoundHandle : u32 { Invalid = 0 };

// ---------------------------------------------------------------------------
// SoundDesc — parameters for playing a sound.
// ---------------------------------------------------------------------------
struct SoundDesc {
    std::string assetPath;         ///< path to audio file
    Vec3        position{};        ///< world-space origin (3D sounds)
    bool        is3D{true};
    f32         volume{1.f};       ///< [0,1]
    f32         pitch{1.f};        ///< [0.5, 2.0] typical
    f32         minDistance{5.f};  ///< full volume within this radius (m)
    f32         maxDistance{200.f};///< inaudible beyond this radius (m)
    bool        looping{false};
    f32         dopplerFactor{1.f};
};

// ---------------------------------------------------------------------------
// SoundInstance — runtime state of a playing sound.
// ---------------------------------------------------------------------------
struct SoundInstance {
    SoundHandle handle{SoundHandle::Invalid};
    SoundDesc   desc;
    f32         playbackTime{0};
    bool        active{false};
};

// ---------------------------------------------------------------------------
// BattlefieldAcoustics — simulates reverb, echo and sound occlusion.
//
// In a large open-world military game the acoustic environment is critical:
//  • Echo off of building walls and mountain faces
//  • Low-frequency rumble of distant artillery attenuated by terrain
//  • Suppressor effects on nearby weapon fire
// ---------------------------------------------------------------------------
class BattlefieldAcoustics {
public:
    struct ReverbZone {
        Vec3  centre;
        f32   radius{100.f};
        f32   decayTime{2.f};   ///< RT60 in seconds
        f32   earlyReflections{0.5f};
        f32   density{0.8f};
    };

    void addReverbZone(ReverbZone zone);
    void update(Vec3 listenerPos);

    /// Returns the reverb parameters that should be applied to a sound at pos.
    [[nodiscard]] ReverbZone evaluateAt(Vec3 pos) const;

private:
    std::vector<ReverbZone> m_zones;
    ReverbZone              m_activeReverb;
};

// ===========================================================================
// AudioSystem
// ===========================================================================
class AudioSystem {
public:
    AudioSystem()  = default;
    ~AudioSystem() = default;

    bool initialize();
    void shutdown();

    /// Called every frame with the current listener (camera/player) position
    /// and orientation forward vector.
    void update(f32 dt, Vec3 listenerPos, Vec3 listenerForward);

    // ---- Playback ----------------------------------------------------------
    SoundHandle playSound(const SoundDesc& desc);
    void        stopSound(SoundHandle handle);
    void        stopAll();
    void        setVolume(SoundHandle handle, f32 volume);
    void        setPitch(SoundHandle handle,  f32 pitch);

    [[nodiscard]] bool          isPlaying(SoundHandle handle) const;
    [[nodiscard]] BattlefieldAcoustics& acoustics() { return m_acoustics; }

private:
    /// Calculate attenuation for a 3D sound (inverse square law with min/max).
    [[nodiscard]] f32 calcAttenuation(const SoundDesc& desc,
                                      Vec3 listenerPos) const;

    std::unordered_map<u32, SoundInstance> m_instances;
    BattlefieldAcoustics                   m_acoustics;
    Vec3                                   m_listenerPos{};
    Vec3                                   m_listenerForward{0,0,-1};

    u32 m_nextHandleID{1};
};

} // namespace shooter
