#include "audio/AudioSystem.h"
#include "core/Logger.h"
#include <vector>

namespace shooter {

struct AudioSystem::Impl {
    std::vector<AudioEmitter> emitters;
    AudioListener             listener;
    bool                      initialised{false};
    u32                       nextId{0};
};

static AudioSystem s_audio;

AudioSystem& AudioSystem::get() { return s_audio; }

bool AudioSystem::init() {
    m_impl = std::make_unique<Impl>();
    m_impl->initialised = true;
    LOG_INFO("AudioSystem", "Initialised (stub backend)");
    // A full implementation would open an OS audio device via
    // platform APIs (XAudio2, ALSA/PipeWire, CoreAudio).
    return true;
}

void AudioSystem::shutdown() {
    if (m_impl) {
        m_impl->emitters.clear();
        m_impl->initialised = false;
    }
    LOG_INFO("AudioSystem", "Shutdown");
}

void AudioSystem::update(const AudioListener& listener, float /*dt*/) {
    if (!m_impl) return;
    m_impl->listener = listener;
    // In a full implementation: update 3D panning, apply HRTF, mix.
}

u32 AudioSystem::playSound(u32 assetId, float x, float y, float z,
                            float volume, float maxDist) {
    if (!m_impl) return UINT32_MAX;
    AudioEmitter e;
    e.soundAssetId  = assetId;
    e.x = x; e.y = y; e.z = z;
    e.volume        = volume;
    e.maxDistance   = maxDist;
    e.playing       = true;
    u32 id = m_impl->nextId++;
    m_impl->emitters.push_back(e);
    return id;
}

u32 AudioSystem::startLoop(u32 assetId, float x, float y, float z,
                            float volume) {
    if (!m_impl) return UINT32_MAX;
    AudioEmitter e;
    e.soundAssetId = assetId;
    e.x = x; e.y = y; e.z = z;
    e.volume = volume;
    e.loop   = true;
    e.playing = true;
    u32 id = m_impl->nextId++;
    m_impl->emitters.push_back(e);
    return id;
}

void AudioSystem::stopSound(u32 id) {
    if (!m_impl || id >= m_impl->emitters.size()) return;
    m_impl->emitters[id].playing = false;
}

void AudioSystem::setEmitterPosition(u32 id, float x, float y, float z) {
    if (!m_impl || id >= m_impl->emitters.size()) return;
    m_impl->emitters[id].x = x;
    m_impl->emitters[id].y = y;
    m_impl->emitters[id].z = z;
}

void AudioSystem::setVolume(u32 id, float volume) {
    if (!m_impl || id >= m_impl->emitters.size()) return;
    m_impl->emitters[id].volume = volume;
}

u32 AudioSystem::activeEmitterCount() const {
    if (!m_impl) return 0;
    u32 n = 0;
    for (const auto& e : m_impl->emitters) n += e.playing ? 1 : 0;
    return n;
}

} // namespace shooter
