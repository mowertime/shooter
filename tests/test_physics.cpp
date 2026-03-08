#include "physics/PhysicsWorld.h"
#include "physics/DestructionSystem.h"
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
#define ASSERT_NEAR(a, b, tol) ASSERT(std::fabs((float)(a)-(float)(b)) < (float)(tol))

void test_physics() {
    g_suiteName = "Physics";
    std::printf("\n--- Physics ---\n");

    // A body falls under gravity and bounces off y=0 ground plane.
    {
        PhysicsWorld world;
        u32 id = world.addRigidBody(0, 10, 0, 1.0f);
        ASSERT(id != UINT32_MAX);

        // Simulate 1 second at 16ms steps.
        for (int i = 0; i < 62; ++i) world.step(0.016f);

        float x, y, z;
        bool ok = world.getPosition(id, x, y, z);
        ASSERT(ok);
        // After 1 second the body should have fallen and bounced; y >= 0.
        ASSERT(y >= 0.0f);
    }

    // Impulse changes velocity.
    {
        PhysicsWorld world;
        u32 id = world.addRigidBody(0, 5, 0, 10.0f);
        world.applyImpulse(id, 100.0f, 0.0f, 0.0f);
        world.step(0.016f);
        float x, y, z;
        world.getPosition(id, x, y, z);
        ASSERT(x > 0.0f); // Moved in +X direction
    }

    // Set and get position.
    {
        PhysicsWorld world;
        u32 id = world.addRigidBody(0, 0, 0, 5.0f);
        world.setPosition(id, 100.0f, 50.0f, 200.0f);
        float x, y, z;
        bool ok = world.getPosition(id, x, y, z);
        ASSERT(ok);
        ASSERT_NEAR(x, 100.0f, 0.01f);
        ASSERT_NEAR(y, 50.0f,  0.01f);
        ASSERT_NEAR(z, 200.0f, 0.01f);
    }

    // Remove body.
    {
        PhysicsWorld world;
        u32 id = world.addRigidBody(0, 1, 0, 1.0f);
        world.removeBody(id);
        float x, y, z;
        bool ok = world.getPosition(id, x, y, z);
        ASSERT(!ok); // Should fail after removal
    }

    // Raycast hits body.
    {
        PhysicsWorld world;
        u32 id = world.addRigidBody(0, 5, 10, 1.0f); // Body at (0,5,10)
        float hitDist;
        u32 hit = world.raycast(0, 5, 0,  // Origin
                                0, 0, 1,  // Direction
                                50.0f, hitDist);
        ASSERT(hit == id);
        ASSERT(hitDist < 11.0f);
    }

    // Destruction system.
    {
        std::vector<DestructibleChunk> chunks(4);
        for (auto& c : chunks) c.health = 100.0f;
        DestructionSystem ds;
        u32 sid = ds.addStructure(std::move(chunks));
        ASSERT(ds.structureCount() == 1);
        bool destroyed = ds.applyDamage(sid, 0, 101.0f);
        ASSERT(destroyed);
        destroyed = ds.applyDamage(sid, 0, 1.0f); // Already destroyed
        ASSERT(!destroyed);
    }

    std::printf("  Physics tests complete\n");
}
