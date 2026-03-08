#pragma once

// ---------------------------------------------------------------------------
// Engine.h — Top-level engine class; owns all subsystems.
// ---------------------------------------------------------------------------

#include <memory>
#include <string>

#include "engine/platform/Platform.h"
#include "engine/platform/Window.h"
#include "engine/platform/Threading.h"
#include "engine/renderer/Renderer.h"
#include "engine/ecs/ECS.h"
#include "engine/physics/Physics.h"
#include "engine/world/World.h"
#include "engine/ai/AI.h"
#include "engine/animation/Animation.h"
#include "engine/audio/Audio.h"
#include "engine/networking/Network.h"

namespace shooter {

// ---------------------------------------------------------------------------
// EngineConfig — all tunable settings for an engine session.
// ---------------------------------------------------------------------------
struct EngineConfig {
    // Window
    u32         windowWidth{1920};
    u32         windowHeight{1080};
    std::string windowTitle{"Shooter"};
    bool        fullscreen{false};

    // Renderer
    RendererAPI rendererAPI{RendererAPI::Vulkan};
    bool        enableValidationLayers{true};  ///< Vulkan debug layers

    // World
    f32  worldSizeKm2{200.f};
    f32  chunkSizeMetres{512.f};
    u32  worldSeed{12345};

    // AI
    u32  maxAIUnits{500};

    // Physics
    bool enablePhysics{true};

    // Networking
    bool enableNetworking{false};
    NetworkRole networkRole{NetworkRole::Standalone};
    u16  networkPort{7777};

    // Threading
    u32 numWorkerThreads{0}; ///< 0 = hardware_concurrency - 1
};

// ===========================================================================
// Engine — the main application object.
//
// Typical usage:
//
//   Engine engine;
//   EngineConfig cfg;
//   cfg.worldSizeKm2 = 200.f;
//   cfg.maxAIUnits   = 500;
//   engine.initialize(cfg);
//   engine.run();         // blocks until quit
//   engine.shutdown();
// ===========================================================================
class Engine {
public:
    Engine()  = default;
    ~Engine() = default;

    // Non-copyable / non-movable (singleton-like, owns all state)
    Engine(const Engine&)            = delete;
    Engine& operator=(const Engine&) = delete;

    bool initialize(const EngineConfig& config);
    void run();    ///< main loop — returns when window is closed / quit is requested
    void shutdown();
    void requestQuit() { m_running = false; }

    // ---- Subsystem accessors -----------------------------------------------
    [[nodiscard]] Renderer*      getRenderer()  { return m_renderer.get(); }
    [[nodiscard]] ECSWorld*      getWorld()     { return m_ecsWorld.get(); }
    [[nodiscard]] PhysicsWorld*  getPhysics()   { return m_physics.get(); }
    [[nodiscard]] World*         getGameWorld() { return m_world.get(); }
    [[nodiscard]] AIManager*     getAI()        { return m_ai.get(); }
    [[nodiscard]] AnimationSystem* getAnimation(){ return m_animation.get(); }
    [[nodiscard]] AudioSystem*   getAudio()     { return m_audio.get(); }
    [[nodiscard]] NetworkManager* getNetwork()  { return m_network.get(); }
    [[nodiscard]] JobSystem*     getJobs()      { return m_jobSystem.get(); }
    [[nodiscard]] const EngineConfig& config() const { return m_config; }

private:
    void tick(f32 dt);  ///< one simulation + render frame

    EngineConfig m_config;
    bool         m_running{false};

    // Subsystems (owned by the engine, initialised in dependency order)
    std::unique_ptr<IWindow>         m_window;
    std::unique_ptr<JobSystem>       m_jobSystem;
    std::unique_ptr<Renderer>        m_renderer;
    std::unique_ptr<ECSWorld>        m_ecsWorld;
    std::unique_ptr<PhysicsWorld>    m_physics;
    std::unique_ptr<World>           m_world;
    std::unique_ptr<AIManager>       m_ai;
    std::unique_ptr<AnimationSystem> m_animation;
    std::unique_ptr<AudioSystem>     m_audio;
    std::unique_ptr<NetworkManager>  m_network;

    // Fixed-step accumulator for deterministic physics/AI.
    static constexpr f32 FIXED_DT = 1.f / 60.f;
    f32 m_accumulator{0.f};
};

} // namespace shooter
