#pragma once

#include "core/Platform.h"

namespace shooter {

/// Current weather conditions.
struct WeatherState {
    float windX{0}, windY{0}, windZ{0}; ///< Wind velocity (m/s)
    float rainIntensity{0.0f};          ///< 0..1
    float snowIntensity{0.0f};
    float fogDensity{0.0f};
    float cloudCover{0.3f};             ///< 0..1
    float temperature{20.0f};           ///< °C
    float visibility{10000.0f};         ///< metres
};

/// Time-of-day state.
struct TimeOfDay {
    float hour{12.0f};    ///< 0..24
    float dayLength{1200.0f}; ///< Real seconds per game day
    bool  isPaused{false};

    float sunElevationRad() const;
    float moonPhase() const; ///< 0..1
};

/// Dynamic weather and day/night cycle controller.
///
/// Weather transitions using a Markov chain of named weather presets.
class WeatherSystem {
public:
    explicit WeatherSystem(u32 seed = 42);

    void update(float dt);

    const WeatherState& weather()   const { return m_current; }
    const TimeOfDay&    timeOfDay() const { return m_tod;     }

    void setTimeScale(float s) { m_timeScale = s; }

    /// Force a specific weather preset (used for story missions).
    void forceWeather(const WeatherState& w) { m_current = w; }

private:
    WeatherState m_current;
    WeatherState m_target;
    TimeOfDay    m_tod;
    float        m_blendTimer{0};
    float        m_blendDuration{300.0f}; ///< seconds to transition
    float        m_timeScale{1.0f};
    u32          m_seed;
};

} // namespace shooter
