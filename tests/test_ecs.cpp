#include "ecs/EntityRegistry.h"
#include <cstdio>
#include <cmath>
#include <vector>

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

void test_ecs() {
    g_suiteName = "ECS";
    std::printf("\n--- ECS ---\n");

    // Create and destroy entities.
    {
        EntityRegistry reg;
        ASSERT(reg.entityCount() == 0);
        Entity e = reg.createEntity();
        ASSERT(e.isValid());
        ASSERT(reg.entityCount() == 1);
        ASSERT(reg.isAlive(e));
        reg.destroyEntity(e);
        ASSERT(!reg.isAlive(e));
        ASSERT(reg.entityCount() == 0);
    }

    // Versioned handles prevent use-after-free.
    {
        EntityRegistry reg;
        Entity e1 = reg.createEntity();
        reg.destroyEntity(e1);
        Entity e2 = reg.createEntity();
        ASSERT(e1.index() == e2.index());
        ASSERT(e1.version() != e2.version());
        ASSERT(!reg.isAlive(e1));
        ASSERT(reg.isAlive(e2));
    }

    // Add and retrieve components.
    {
        EntityRegistry reg;
        Entity e = reg.createEntity();
        reg.addComponent(e, TransformComponent{10.0f, 20.0f, 30.0f});
        ASSERT(reg.hasComponent<TransformComponent>(e));
        auto* t = reg.getComponent<TransformComponent>(e);
        ASSERT(t != nullptr);
        ASSERT(t->x == 10.0f);
        ASSERT(t->y == 20.0f);
        ASSERT(t->z == 30.0f);
    }

    // Remove a component.
    {
        EntityRegistry reg;
        Entity e = reg.createEntity();
        reg.addComponent(e, HealthComponent{80.0f, 100.0f});
        ASSERT(reg.hasComponent<HealthComponent>(e));
        reg.removeComponent<HealthComponent>(e);
        ASSERT(!reg.hasComponent<HealthComponent>(e));
    }

    // Multiple components on one entity.
    {
        EntityRegistry reg;
        Entity e = reg.createEntity();
        reg.addComponent(e, TransformComponent{1, 2, 3});
        reg.addComponent(e, HealthComponent{50.0f, 100.0f});
        reg.addComponent(e, RigidBodyComponent{});
        ASSERT(reg.hasComponent<TransformComponent>(e));
        ASSERT(reg.hasComponent<HealthComponent>(e));
        ASSERT(reg.hasComponent<RigidBodyComponent>(e));
        auto* h = reg.getComponent<HealthComponent>(e);
        ASSERT(h && h->current == 50.0f);
        ASSERT(h && h->isAlive());
    }

    // Many entities: create, validate, destroy half.
    {
        EntityRegistry reg;
        constexpr u32 N = 500;
        std::vector<Entity> ents;
        ents.reserve(N);
        for (u32 i = 0; i < N; ++i) {
            Entity e = reg.createEntity();
            reg.addComponent(e, TransformComponent{static_cast<float>(i), 0, 0});
            ents.push_back(e);
        }
        ASSERT(reg.entityCount() == N);
        for (u32 i = 0; i < N; ++i) {
            auto* t = reg.getComponent<TransformComponent>(ents[i]);
            ASSERT(t && t->x == static_cast<float>(i));
        }
        for (u32 i = 0; i < N; i += 2) reg.destroyEntity(ents[i]);
        ASSERT(reg.entityCount() == N / 2);
    }

    // WeaponComponent defaults.
    {
        EntityRegistry reg;
        Entity e = reg.createEntity();
        reg.addComponent(e, WeaponComponent{});
        auto* w = reg.getComponent<WeaponComponent>(e);
        ASSERT(w && w->ammoInMag  == 30);
        ASSERT(w && w->ammoReserve == 120);
    }

    std::printf("  ECS tests complete\n");
}
