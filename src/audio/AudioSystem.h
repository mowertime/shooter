#pragma once

#include "core/Platform.h"
#include <string>
#include <memory>

namespace shooter {

/// Audio emitter (3D sound source).
struct AudioEmitter {
    u32   soundAssetId{UINT32_MAX};
    float x{0}, y{0}, z{0};     ///< World position
    float volume{1.0f};         ///< 0..1
    float pitch{1.0f};
    float maxDistance{100.0f};  ///< Audible distance (m)
    bool  loop{false};
    bool  playing{false};
};

/// Audio listener (player ears).
struct AudioListener {
    float x{0}, y{0}, z{0};
    float forwardX{0}, forwardY{0}, forwardZ{1};
    float upX{0}, upY{1}, upZ{0};
};

/// Audio system: manages sound emitters, mixer, and spatial processing.
///
/// Uses HRTF for binaural headphone rendering and a ray-casting
/// reverb model for environmental acoustics.
class AudioSystem {
public:
    static AudioSystem& get();

    bool init();
    void shutdown();

    /// Update listener transform and process audio graph.
    void update(const AudioListener& listener, float dt);

    /// Play a one-shot sound at world position.
    u32  playSound(u32 assetId, float x, float y, float z,
                   float volume = 1.0f, float maxDist = 100.0f);

    /// Start a looping emitter.
    u32  startLoop(u32 assetId, float x, float y, float z,
                   float volume = 1.0f);

    void stopSound(u32 emitterId);
    void setEmitterPosition(u32 emitterId, float x, float y, float z);
    void setVolume(u32 emitterId, float volume);

    u32  activeEmitterCount() const;

    AudioSystem() = default;
    ~AudioSystem() = default;

private:

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace shooter
