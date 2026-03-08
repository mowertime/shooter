// ---------------------------------------------------------------------------
// Engine.cpp — Engine initialization, main loop, and shutdown.
// ---------------------------------------------------------------------------

#include "engine/Engine.h"

#include <iostream>
#include <thread>
#include <chrono>

namespace shooter {

bool Engine::initialize(const EngineConfig& config) {
    m_config = config;

    std::cout << "=== Shooter Engine Initializing ===\n";

    // ---- 1. Job system (platform threads) ----------------------------------
    m_jobSystem = std::make_unique<JobSystem>(config.numWorkerThreads);

    // ---- 2. Window ---------------------------------------------------------
    m_window = WindowFactory::create();
    WindowDesc wd;
    wd.title      = config.windowTitle;
    wd.width      = config.windowWidth;
    wd.height     = config.windowHeight;
    wd.fullscreen = config.fullscreen;
    if (!m_window->create(wd)) {
        std::cerr << "[Engine] Failed to create window\n";
        return false;
    }

    // ---- 3. Renderer -------------------------------------------------------
    m_renderer = std::make_unique<Renderer>();
    RendererDesc rd;
    rd.api                  = config.rendererAPI;
    rd.nativeWindowHandle   = m_window->getNativeHandle();
    rd.width                = config.windowWidth;
    rd.height               = config.windowHeight;
    rd.enableValidation     = config.enableValidationLayers;
    if (!m_renderer->initialize(rd)) {
        std::cerr << "[Engine] Failed to initialize renderer\n";
        return false;
    }

    // ---- 4. ECS world ------------------------------------------------------
    m_ecsWorld = std::make_unique<ECSWorld>();

    // ---- 5. Physics --------------------------------------------------------
    if (config.enablePhysics) {
        m_physics = std::make_unique<PhysicsWorld>();
        if (!m_physics->initialize()) {
            std::cerr << "[Engine] Failed to initialize physics\n";
            return false;
        }
    }

    // ---- 6. Game world (terrain + streaming) --------------------------------
    m_world = std::make_unique<World>();
    WorldDesc worldDesc;
    worldDesc.sizeKm2         = config.worldSizeKm2;
    worldDesc.chunkSizeMetres = config.chunkSizeMetres;
    worldDesc.seed            = config.worldSeed;
    worldDesc.name            = config.windowTitle;
    if (!m_world->initialize(worldDesc)) {
        std::cerr << "[Engine] Failed to initialize world\n";
        return false;
    }

    // ---- 7. AI manager -----------------------------------------------------
    m_ai = std::make_unique<AIManager>();
    if (!m_ai->initialize(config.maxAIUnits)) {
        std::cerr << "[Engine] Failed to initialize AI\n";
        return false;
    }

    // ---- 8. Animation system ------------------------------------------------
    m_animation = std::make_unique<AnimationSystem>();
    if (!m_animation->initialize()) {
        std::cerr << "[Engine] Failed to initialize animation\n";
        return false;
    }

    // ---- 9. Audio ----------------------------------------------------------
    m_audio = std::make_unique<AudioSystem>();
    if (!m_audio->initialize()) {
        std::cerr << "[Engine] Failed to initialize audio\n";
        return false;
    }

    // ---- 10. Networking (optional) -----------------------------------------
    if (config.enableNetworking) {
        m_network = std::make_unique<NetworkManager>();
        if (!m_network->initialize(config.networkRole, config.networkPort)) {
            std::cerr << "[Engine] Failed to initialize networking\n";
            return false;
        }
    }

    m_running = true;
    std::cout << "=== Engine Initialized Successfully ===\n";
    return true;
}

void Engine::run() {
    std::cout << "[Engine] Entering main loop\n";

    Timer frameTimer;
    frameTimer.reset();

    // We run for a few demo frames in the prototype instead of forever.
    // A real game would loop until m_running == false.
    constexpr u32 DEMO_FRAMES = 5;
    u32 frameCount = 0;

    while (m_running && frameCount < DEMO_FRAMES) {
        f64 elapsed = frameTimer.elapsedSeconds();
        frameTimer.reset();
        f32 dt = static_cast<f32>(elapsed);
        // Clamp dt to prevent spiral-of-death on slow frames.
        if (dt > 0.25f) dt = 0.25f;

        // Poll OS events.
        m_window->pollEvents();
        if (m_window->shouldClose()) {
            m_running = false;
            break;
        }

        // Fixed-timestep accumulator for physics / AI.
        m_accumulator += dt;
        while (m_accumulator >= FIXED_DT) {
            tick(FIXED_DT);
            m_accumulator -= FIXED_DT;
        }

        // Variable-rate render at the actual frame rate.
        m_renderer->beginFrame();
        // TODO: submit draw calls from scene
        m_renderer->endFrame();

        ++frameCount;

        // Simulate ~60 fps in the prototype.
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    std::cout << "[Engine] Main loop exited after " << frameCount << " frame(s)\n";
}

void Engine::tick(f32 dt) {
    // Camera position (placeholder — in a real build comes from PlayerController).
    static f32 camX = 0.f, camZ = 0.f;
    camX += 1.f * dt; // slowly pan for demo

    // World streaming.
    if (m_world)   m_world->update(dt, camX, camZ);

    // Physics fixed step.
    if (m_physics) m_physics->step(dt);

    // AI (LOD-aware update for all agents).
    if (m_ai)      m_ai->update(*m_ecsWorld, dt, {camX, 0.f, camZ});

    // Animation.
    if (m_animation) m_animation->update(dt);

    // Audio listener position.
    if (m_audio)   m_audio->update(dt, {camX, 0.f, camZ}, {0,0,1});

    // Networking.
    if (m_network) m_network->update(*m_ecsWorld, dt);
}

void Engine::shutdown() {
    std::cout << "=== Engine Shutting Down ===\n";

    // Shutdown in reverse-initialization order.
    if (m_network)   m_network->shutdown();
    if (m_audio)     m_audio->shutdown();
    if (m_animation) m_animation->shutdown();
    if (m_ai)        m_ai->shutdown();
    if (m_world)     m_world->shutdown();
    if (m_physics)   m_physics->shutdown();
    if (m_renderer)  m_renderer->shutdown();
    if (m_window)    m_window->destroy();

    m_network.reset();
    m_audio.reset();
    m_animation.reset();
    m_ai.reset();
    m_world.reset();
    m_physics.reset();
    m_ecsWorld.reset();
    m_renderer.reset();
    m_window.reset();
    m_jobSystem.reset();

    std::cout << "=== Engine Shutdown Complete ===\n";
}

} // namespace shooter
