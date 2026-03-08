#include "audio/AudioSystem.h"
#include "audio/SpatialAudio.h"
#include <cstdio>
#include <cmath>

using namespace shooter;

extern int g_pass;
extern int g_fail;
extern const char* g_suiteName;

#define ASSERT(expr) \
    do { \
        if (!(expr)) { \
            std::fprintf(stderr, "  FAIL [%s] %s:%d  (%s)\n", \
                         g_suiteName, __FILE__, __LINE__, #expr); \
            ++g_fail; \
        } else { \
            ++g_pass; \
        } \
    } while(0)
#define ASSERT_NEAR(a,b,tol) ASSERT(std::fabs((float)(a)-(float)(b))<(float)(tol))

void test_audio() {
    g_suiteName = "Audio";
    std::printf("\n--- Audio ---\n");

    // Init / shutdown cycle.
    {
        ASSERT(AudioSystem::get().init());
        ASSERT(AudioSystem::get().activeEmitterCount() == 0);
        AudioSystem::get().shutdown();
    }

    // Play and stop a sound.
    {
        AudioSystem::get().init();
        u32 id = AudioSystem::get().playSound(1, 0, 0, 0, 1.0f, 100.0f);
        ASSERT(id != UINT32_MAX);
        ASSERT(AudioSystem::get().activeEmitterCount() == 1);
        AudioSystem::get().stopSound(id);
        ASSERT(AudioSystem::get().activeEmitterCount() == 0);
        AudioSystem::get().shutdown();
    }

    // SpatialAudio: attenuation.
    {
        ASSERT_NEAR(SpatialAudio::attenuate(0.0f, 100.0f),   1.0f, 0.001f);
        ASSERT_NEAR(SpatialAudio::attenuate(100.0f, 100.0f), 0.0f, 0.001f);
        ASSERT_NEAR(SpatialAudio::attenuate(200.0f, 100.0f), 0.0f, 0.001f);
        float mid = SpatialAudio::attenuate(50.0f, 100.0f);
        ASSERT(mid > 0.0f && mid < 1.0f);
    }

    // SpatialAudio: equal-power pan sums to ~1.
    {
        float L, R;
        SpatialAudio::equalPowerPan(0.0f, L, R);
        // Centre: both channels equal
        ASSERT_NEAR(L, R, 0.01f);
        // Total power ≈ 1.
        ASSERT_NEAR(L * L + R * R, 1.0f, 0.02f);
    }

    // SpatialAudio: pan angle computation.
    {
        // Sound directly to the right of the listener.
        float angle = SpatialAudio::computePanAngle(
            10.0f, 0.0f,   // source
             0.0f, 0.0f,   // listener
             0.0f, 1.0f);  // forward = +Z
        ASSERT(angle > 0.0f); // Should be positive (right)
    }

    std::printf("  Audio tests complete\n");
}
