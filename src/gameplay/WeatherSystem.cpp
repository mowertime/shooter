#include "gameplay/WeatherSystem.h"
#include <cmath>
#include <algorithm>

namespace shooter {

static float randLcg(u32& state) {
    state = state * 1664525u + 1013904223u;
    return static_cast<float>(state & 0xFFFFu) / 65535.0f;
}

WeatherSystem::WeatherSystem(u32 seed) : m_seed(seed) {
    m_current.windX = 2.0f;
    m_current.cloudCover = 0.3f;
}

void WeatherSystem::update(float dt) {
    // Advance time of day.
    if (!m_tod.isPaused) {
        m_tod.hour += dt * m_timeScale * (24.0f / m_tod.dayLength);
        if (m_tod.hour >= 24.0f) m_tod.hour -= 24.0f;
    }

    // Blend towards target weather.
    m_blendTimer += dt;
    float t = std::min(m_blendTimer / m_blendDuration, 1.0f);
    auto lerp = [t](float a, float b) { return a + (b - a) * t; };

    m_current.windX       = lerp(m_current.windX,       m_target.windX);
    m_current.windZ       = lerp(m_current.windZ,       m_target.windZ);
    m_current.rainIntensity = lerp(m_current.rainIntensity, m_target.rainIntensity);
    m_current.fogDensity  = lerp(m_current.fogDensity,  m_target.fogDensity);
    m_current.cloudCover  = lerp(m_current.cloudCover,  m_target.cloudCover);

    // Trigger new weather transition after blending completes.
    if (m_blendTimer >= m_blendDuration) {
        m_blendTimer = 0.0f;
        // Pick a new random target.
        m_target.rainIntensity = randLcg(m_seed) * 0.7f;
        m_target.windX         = (randLcg(m_seed) - 0.5f) * 20.0f;
        m_target.windZ         = (randLcg(m_seed) - 0.5f) * 20.0f;
        m_target.fogDensity    = randLcg(m_seed) * 0.05f;
        m_target.cloudCover    = randLcg(m_seed);
        m_blendDuration        = 120.0f + randLcg(m_seed) * 600.0f;
    }
}

float TimeOfDay::sunElevationRad() const {
    // Simplified sun angle: sunrise at 6h, noon at 12h, sunset at 18h.
    return std::sin((hour - 6.0f) / 12.0f * 3.14159265f);
}

float TimeOfDay::moonPhase() const {
    return std::fmod(hour / 24.0f, 1.0f);
}

} // namespace shooter
