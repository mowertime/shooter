// ---------------------------------------------------------------------------
// Audio.cpp — AudioSystem implementation (stub — no actual audio API calls).
//
// A production implementation would integrate OpenAL / FMOD / Wwise here.
// All the data structures and attenuation math are real.
// ---------------------------------------------------------------------------

#include "engine/audio/Audio.h"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace shooter {

// ===========================================================================
// BattlefieldAcoustics
// ===========================================================================

void BattlefieldAcoustics::addReverbZone(ReverbZone zone) {
    m_zones.push_back(std::move(zone));
}

void BattlefieldAcoustics::update(Vec3 listenerPos) {
    // Find which zone the listener is inside; blend up to two zones.
    // TODO: proper multi-zone crossfade
    for (const auto& zone : m_zones) {
        Vec3 diff = listenerPos - zone.centre;
        f32  dist = diff.length();
        if (dist < zone.radius) {
            m_activeReverb = zone;
            return;
        }
    }
    // Default: open outdoor space — very short reverb.
    m_activeReverb = { listenerPos, 9999.f, 0.3f, 0.1f, 0.2f };
}

BattlefieldAcoustics::ReverbZone
BattlefieldAcoustics::evaluateAt(Vec3 pos) const {
    for (const auto& zone : m_zones) {
        Vec3 diff = pos - zone.centre;
        if (diff.length() < zone.radius) return zone;
    }
    return { pos, 9999.f, 0.3f, 0.1f, 0.2f };
}

// ===========================================================================
// AudioSystem
// ===========================================================================

bool AudioSystem::initialize() {
    // TODO: initialise audio backend (OpenAL, FMOD, etc.)
    std::cout << "[Audio] Initialized (stub)\n";
    return true;
}

void AudioSystem::shutdown() {
    stopAll();
    m_instances.clear();
    std::cout << "[Audio] Shutdown\n";
}

void AudioSystem::update(f32 dt, Vec3 listenerPos, Vec3 listenerForward) {
    m_listenerPos     = listenerPos;
    m_listenerForward = listenerForward;
    m_acoustics.update(listenerPos);

    // Update all active instances: advance playback time, apply attenuation.
    for (auto it = m_instances.begin(); it != m_instances.end(); ) {
        SoundInstance& inst = it->second;
        if (!inst.active) { it = m_instances.erase(it); continue; }

        inst.playbackTime += dt;

        // TODO: feed updated position + attenuation to the audio backend.
        f32 atten = inst.desc.is3D ? calcAttenuation(inst.desc, listenerPos) : 1.f;
        (void)atten;

        // Remove non-looping sounds that have finished.
        // TODO: use actual clip duration from the audio asset.
        if (!inst.desc.looping && inst.playbackTime > 10.f /* placeholder duration */) {
            inst.active = false;
        }

        ++it;
    }
}

SoundHandle AudioSystem::playSound(const SoundDesc& desc) {
    SoundHandle handle = static_cast<SoundHandle>(m_nextHandleID++);
    SoundInstance inst;
    inst.handle  = handle;
    inst.desc    = desc;
    inst.active  = true;
    // TODO: submit to audio backend
    m_instances[static_cast<u32>(handle)] = std::move(inst);
    return handle;
}

void AudioSystem::stopSound(SoundHandle handle) {
    auto it = m_instances.find(static_cast<u32>(handle));
    if (it != m_instances.end()) {
        it->second.active = false;
        // TODO: stop in audio backend
    }
}

void AudioSystem::stopAll() {
    for (auto& [id, inst] : m_instances) {
        inst.active = false;
        // TODO: stop in audio backend
    }
}

void AudioSystem::setVolume(SoundHandle handle, f32 volume) {
    auto it = m_instances.find(static_cast<u32>(handle));
    if (it != m_instances.end()) {
        it->second.desc.volume = std::clamp(volume, 0.f, 1.f);
        // TODO: update in audio backend
    }
}

void AudioSystem::setPitch(SoundHandle handle, f32 pitch) {
    auto it = m_instances.find(static_cast<u32>(handle));
    if (it != m_instances.end()) {
        it->second.desc.pitch = std::clamp(pitch, 0.1f, 4.f);
        // TODO: update in audio backend
    }
}

bool AudioSystem::isPlaying(SoundHandle handle) const {
    auto it = m_instances.find(static_cast<u32>(handle));
    return it != m_instances.end() && it->second.active;
}

f32 AudioSystem::calcAttenuation(const SoundDesc& desc, Vec3 listenerPos) const {
    Vec3 diff = listenerPos - desc.position;
    f32  dist = diff.length();
    if (dist <= desc.minDistance) return 1.f;
    if (dist >= desc.maxDistance) return 0.f;
    // Inverse-distance rolloff (OpenAL default model):
    //   gain = minDistance / (minDistance + rolloffFactor * (dist - minDistance))
    constexpr f32 rolloffFactor = 1.f;
    return desc.minDistance /
        (desc.minDistance + rolloffFactor * (dist - desc.minDistance));
}

} // namespace shooter
