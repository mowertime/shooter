/**
 * ShooterEngine – Main entry point.
 *
 * Initialises all engine subsystems in dependency order, then runs the
 * main game loop until the window requests a close.  Each frame:
 *   1. Poll OS events / player input.
 *   2. Tick the physics simulation.
 *   3. Update AI controllers and squads.
 *   4. Tick gameplay systems (player, weapons, weather, battle).
 *   5. Stream world tiles around the player.
 *   6. Submit the frame to the renderer.
 */

#include "core/Platform.h"
#include "core/Logger.h"
#include "core/Window.h"
#include "core/FileSystem.h"
#include "core/Threading.h"
#include "core/Memory.h"

#include "ecs/EntityRegistry.h"

#include "renderer/Renderer.h"

#include "physics/PhysicsWorld.h"
#include "physics/VehiclePhysics.h"
#include "physics/ProjectileSimulator.h"
#include "physics/DestructionSystem.h"

#include "ai/AIController.h"
#include "ai/SquadManager.h"
#include "ai/PerceptionSystem.h"
#include "ai/CoverSystem.h"

#include "world/WorldSystem.h"
#include "world/WorldStreamer.h"
#include "world/ProceduralCity.h"

#include "audio/AudioSystem.h"

#include "network/NetworkSystem.h"

#include "gameplay/Player.h"
#include "gameplay/Weapon.h"
#include "gameplay/WeatherSystem.h"
#include "gameplay/BattleSystem.h"

#include "animation/AnimationSystem.h"

#include <memory>
#include <cstdio>

using namespace shooter;

int main() {
    // ── Platform ──────────────────────────────────────────────────────────────
    PlatformInfo platformInfo{};
    if (!platformInit(platformInfo)) {
        LOG_FATAL("Main", "Platform init failed");
        return 1;
    }
    LOG_INFO("Main", "Platform: %s, cores=%u, RAM=%llu MB",
             platformInfo.name,
             platformInfo.logicalCoreCount,
             static_cast<unsigned long long>(platformInfo.totalPhysicalMemoryBytes / (1024*1024)));

    // ── Worker thread pool ────────────────────────────────────────────────────
    ThreadPool workerPool(platformInfo.logicalCoreCount - 1);
    LOG_INFO("Main", "Worker threads: %u", workerPool.workerCount());

    // ── File system ───────────────────────────────────────────────────────────
    FileSystem::get().mount("./data", "data");

    // ── Window ────────────────────────────────────────────────────────────────
    WindowDesc wdesc;
    wdesc.title    = "Shooter – Next-Gen Combat Engine";
    wdesc.width    = 1920;
    wdesc.height   = 1080;
    wdesc.fullscreen = false;
    std::unique_ptr<Window> window(Window::create(wdesc));

    // ── Renderer ──────────────────────────────────────────────────────────────
    RendererDesc rdesc;
    rdesc.backend         = platformInfo.supportsVulkan
                                ? RendererBackend::Vulkan
                                : RendererBackend::Stub;
    rdesc.window          = window.get();
    rdesc.enableValidation = false;
    rdesc.enableRaytracing = false;
    auto renderer = Renderer::create(rdesc);
    LOG_INFO("Main", "Renderer backend: %s", renderer->backendName());

    // ── ECS world ─────────────────────────────────────────────────────────────
    EntityRegistry registry;
    registry.addSystem(std::make_unique<PhysicsIntegrationSystem>());
    registry.addSystem(std::make_unique<AIUpdateSystem>());
    registry.addSystem(std::make_unique<WeaponSystem>());

    // ── Physics ───────────────────────────────────────────────────────────────
    PhysicsWorld   physics;
    VehiclePhysics vehiclePhysics;
    AircraftPhysics aircraftPhysics;
    ProjectileSimulator projSim;
    DestructionSystem   destructSys;

    projSim.setHitCallback([](const ProjectileHit& hit) {
        LOG_DEBUG("Ballistics", "Hit at (%.1f, %.1f, %.1f) vel=%.0f m/s",
                  hit.px, hit.py, hit.pz, hit.remainingVelocity);
    });

    // ── World ─────────────────────────────────────────────────────────────────
    WorldDesc wld;
    wld.widthKm  = 200.0f;
    wld.heightKm = 200.0f;
    wld.seed     = 12345;
    WorldSystem world(wld);
    world.init();

    // Place a procedural city near the centre of the map.
    world.cities().generate(100'000.0f, 100'000.0f, 5000.0f, wld.seed, 300);

    // ── Audio ─────────────────────────────────────────────────────────────────
    AudioSystem::get().init();

    // ── Network ───────────────────────────────────────────────────────────────
    NetworkDesc netDesc;
    netDesc.role = NetworkRole::Offline;
    NetworkSystem::get().init(netDesc);

    // ── Gameplay ──────────────────────────────────────────────────────────────
    Player player(registry, physics);
    float spawnY = world.sampleHeight(100'000.0f, 100'000.0f) + 1.8f;
    player.setPosition(100'000.0f, spawnY, 100'000.0f);

    Weapon primaryWeapon(WeaponProfiles::m4a1());
    WeatherSystem weather(wld.seed);
    BattleSystem  battleSys;

    // ── AI population ─────────────────────────────────────────────────────────
    AIControllerPool aiPool;
    SquadManager     squadMgr;
    PerceptionSystem perception;
    CoverSystem      coverSys;

    battleSys.addSquadManager(&squadMgr);

    // Spawn two opposing squads.
    u32 friendlySquadId = squadMgr.createSquad(0);
    u32 enemySquadId    = squadMgr.createSquad(1);
    Squad* friendlySquad = squadMgr.getSquad(friendlySquadId);
    Squad* enemySquad    = squadMgr.getSquad(enemySquadId);

    if (friendlySquad) friendlySquad->setObjective(100'100.0f, spawnY, 100'050.0f);
    if (enemySquad)    enemySquad->setObjective   (100'200.0f, spawnY, 100'200.0f);
    if (enemySquad)    enemySquad->setState(SquadState::Assault);

    constexpr u32 SQUAD_SIZE = 6;
    for (u32 i = 0; i < SQUAD_SIZE; ++i) {
        Entity e = registry.createEntity();
        registry.addComponent(e, TransformComponent{
            100'000.0f + static_cast<float>(i) * 3.0f, spawnY, 100'100.0f});
        registry.addComponent(e, HealthComponent{100.0f, 100.0f});
        AIComponent aic;
        aic.factionId    = 0;
        aic.controllerId = aiPool.create(e.index(), 0);
        registry.addComponent(e, aic);
        perception.registerAgent(e.index());
        if (friendlySquad) friendlySquad->addMember(aic.controllerId);
    }
    for (u32 i = 0; i < SQUAD_SIZE; ++i) {
        Entity e = registry.createEntity();
        registry.addComponent(e, TransformComponent{
            100'200.0f + static_cast<float>(i) * 3.0f, spawnY, 100'200.0f});
        registry.addComponent(e, HealthComponent{100.0f, 100.0f});
        AIComponent aic;
        aic.factionId    = 1;
        aic.controllerId = aiPool.create(e.index(), 1);
        registry.addComponent(e, aic);
        perception.registerAgent(e.index());
        if (enemySquad) enemySquad->addMember(aic.controllerId);
    }

    // ── Animation ─────────────────────────────────────────────────────────────
    AnimationSystem animSys;
    AnimClip idleClip;  idleClip.name = "idle";  idleClip.duration = 1.0f;  idleClip.loop = true;
    AnimClip walkClip;  walkClip.name = "walk";  walkClip.duration = 0.8f;  walkClip.loop = true;
    u32 idleId = animSys.loadClip(idleClip);
    u32 walkId = animSys.loadClip(walkClip);
    animSys.playClip(player.entity().index(), idleId, true);
    (void)walkId;

    // ── BattleSystem front lines ───────────────────────────────────────────────
    FrontLine fl;
    fl.xMin = 99'000.0f; fl.xMax = 101'000.0f; fl.z = 100'150.0f;
    battleSys.addFrontLine(fl);

    // ── Main loop ─────────────────────────────────────────────────────────────
    LOG_INFO("Main", "Engine running – entering main loop");

    i64      lastTime   = platformGetTimeMicros();
    float    gameTime   = 0.0f;
    bool     running    = true;

    constexpr float FIXED_DT   = 1.0f / 60.0f;  // Physics / AI tick
    float           accumulator = 0.0f;

    // For demo: run a fixed number of frames then exit.
    constexpr int MAX_FRAMES = 180; // 3 seconds at 60 Hz
    int frameCount = 0;

    PlayerInput input{};

    while (running && frameCount < MAX_FRAMES) {
        i64   now    = platformGetTimeMicros();
        float dtReal = static_cast<float>(now - lastTime) * 1e-6f;
        lastTime     = now;
        dtReal        = std::min(dtReal, 0.1f); // clamp to prevent spiral of death
        gameTime     += dtReal;
        accumulator  += dtReal;

        // Poll OS events.
        if (!window->pollEvents()) running = false;

        // Simulate input for demo (walk forward slowly).
        input.moveForward = 0.5f;
        input.lookYaw     = 0.0001f;

        // Fixed-step physics & AI.
        while (accumulator >= FIXED_DT) {
            physics.step(FIXED_DT);
            projSim.update(FIXED_DT,
                           weather.weather().windX,
                           weather.weather().windY,
                           weather.weather().windZ);
            registry.updateSystems(FIXED_DT);
            aiPool.updateAll(FIXED_DT);
            squadMgr.updateAll(aiPool, FIXED_DT);
            perception.updateSight(gameTime);
            battleSys.update(FIXED_DT);
            accumulator -= FIXED_DT;
        }

        // Variable-rate updates.
        player.update(input, dtReal);
        weather.update(dtReal);
        {
            float px, py, pz;
            player.getPosition(px, py, pz);
            world.update(px, pz, dtReal);

            AudioListener listener{px, py, pz, 0, 0, 1, 0, 1, 0};
            AudioSystem::get().update(listener, dtReal);
        }
        animSys.update(dtReal);
        NetworkSystem::get().update(dtReal);

        // Render.
        if (!window->isMinimised()) {
            renderer->beginFrame();
            // DrawCalls would be submitted here by mesh/terrain/particle renderers.
            renderer->endFrame();
        }

        ++frameCount;

        if (frameCount % 60 == 0) {
            float px, py, pz;
            player.getPosition(px, py, pz);
            LOG_INFO("Main",
                     "Frame %d | time=%.1fs | player=(%.0f,%.0f,%.0f) "
                     "health=%.0f | tiles=%u | AIs=%u | hour=%.1fh | rain=%.2f",
                     frameCount, gameTime, px, py, pz,
                     player.health(),
                     world.streamer().loadedTileCount(),
                     aiPool.count(),
                     weather.timeOfDay().hour,
                     weather.weather().rainIntensity);
        }
    }

    // ── Shutdown ──────────────────────────────────────────────────────────────
    LOG_INFO("Main", "Shutting down after %d frames", frameCount);

    NetworkSystem::get().shutdown();
    AudioSystem::get().shutdown();
    world.shutdown();
    workerPool.waitIdle();
    renderer->shutdown();
    platformShutdown();

    LOG_INFO("Main", "Clean shutdown");
    return 0;
}
