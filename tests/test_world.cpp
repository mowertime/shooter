#include "world/Terrain.h"
#include "world/BiomeSystem.h"
#include "world/ProceduralCity.h"
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

void test_world() {
    g_suiteName = "World";
    std::printf("\n--- World ---\n");

    // Terrain: height samples in [0, 800] m.
    {
        Terrain t(200'000.0f, 200'000.0f, 256, 42);
        float h = t.sampleHeight(100'000.0f, 100'000.0f);
        ASSERT(h >= 0.0f && h <= 800.0f);

        // Edge samples should be 0 (out-of-bounds).
        float hEdge = t.sampleHeight(-1.0f, -1.0f);
        ASSERT(hEdge == 0.0f);
    }

    // Terrain: surface normal has unit length.
    {
        Terrain t(200'000.0f, 200'000.0f, 256, 7);
        float nx, ny, nz;
        t.sampleNormal(50'000.0f, 50'000.0f, nx, ny, nz);
        float len = std::sqrt(nx*nx + ny*ny + nz*nz);
        ASSERT_NEAR(len, 1.0f, 0.01f);
        ASSERT(ny > 0.0f); // Normal should point upward
    }

    // Different seeds produce different terrain.
    {
        Terrain t1(10'000.0f, 10'000.0f, 64, 1);
        Terrain t2(10'000.0f, 10'000.0f, 64, 99);
        float h1 = t1.sampleHeight(5000.0f, 5000.0f);
        float h2 = t2.sampleHeight(5000.0f, 5000.0f);
        ASSERT(std::fabs(h1 - h2) > 0.1f); // Different seeds → different terrain
    }

    // BiomeSystem: 12 biomes, every point has a valid biome.
    {
        BiomeSystem bs;
        bs.generateBiomes(42, 200'000.0f, 200'000.0f, 12);
        ASSERT(bs.biomeCount() == 12);

        BiomeId b1 = bs.sampleBiome(0, 0);
        BiomeId b2 = bs.sampleBiome(100'000, 100'000);
        ASSERT(static_cast<u8>(b1) <= static_cast<u8>(BiomeId::Urban));
        ASSERT(static_cast<u8>(b2) <= static_cast<u8>(BiomeId::Urban));
    }

    // ProceduralCity: generates expected number of blocks.
    {
        ProceduralCity city;
        city.generate(0, 0, 2000.0f, 42, 100);
        ASSERT(city.blockCount() > 0);
        ASSERT(city.blockCount() <= 100);

        // Blocks should be within the generation radius.
        for (u32 i = 0; i < city.blockCount(); ++i) {
            const CityBlock& blk = city.blocks()[i];
            float d = std::sqrt(blk.x * blk.x + blk.z * blk.z);
            ASSERT(d < 2500.0f); // Allow slight overshoot from grid snap
        }
    }

    std::printf("  World tests complete\n");
}
