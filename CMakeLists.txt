cmake_minimum_required(VERSION 3.20)
project(ShooterEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Build options ─────────────────────────────────────────────────────────────
option(SHOOTER_ENABLE_VULKAN   "Enable Vulkan renderer backend"      ON)
option(SHOOTER_ENABLE_DX12     "Enable DirectX 12 renderer backend"  OFF)
option(SHOOTER_ENABLE_RAYTRACING "Enable ray-tracing support"        OFF)
option(SHOOTER_BUILD_TESTS     "Build unit tests"                     ON)
option(SHOOTER_BUILD_EDITOR    "Build world editor tool"              ON)

# ── Compiler warnings ─────────────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /WX /MP)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# ── Third-party / system packages ─────────────────────────────────────────────
find_package(Threads REQUIRED)

if(SHOOTER_ENABLE_VULKAN)
    find_package(Vulkan QUIET)
    if(Vulkan_FOUND)
        message(STATUS "Vulkan SDK found: ${Vulkan_INCLUDE_DIRS}")
    else()
        message(STATUS "Vulkan SDK not found – renderer will compile in stub mode")
    endif()
endif()

# ── Source file lists ──────────────────────────────────────────────────────────
set(CORE_SOURCES
    src/core/Platform.cpp
    src/core/Window.cpp
    src/core/FileSystem.cpp
    src/core/Threading.cpp
    src/core/Memory.cpp
    src/core/Logger.cpp
)

set(ECS_SOURCES
    src/ecs/EntityRegistry.cpp
)

set(RENDERER_SOURCES
    src/renderer/Renderer.cpp
    src/renderer/VulkanRenderer.cpp
    src/renderer/RenderGraph.cpp
    src/renderer/GpuBuffer.cpp
)

set(PHYSICS_SOURCES
    src/physics/PhysicsWorld.cpp
    src/physics/RigidBody.cpp
    src/physics/VehiclePhysics.cpp
    src/physics/ProjectileSimulator.cpp
    src/physics/DestructionSystem.cpp
)

set(AI_SOURCES
    src/ai/BehaviorTree.cpp
    src/ai/AIController.cpp
    src/ai/SquadManager.cpp
    src/ai/PerceptionSystem.cpp
    src/ai/CoverSystem.cpp
)

set(WORLD_SOURCES
    src/world/WorldSystem.cpp
    src/world/Terrain.cpp
    src/world/WorldStreamer.cpp
    src/world/BiomeSystem.cpp
    src/world/ProceduralCity.cpp
)

set(AUDIO_SOURCES
    src/audio/AudioSystem.cpp
    src/audio/SpatialAudio.cpp
)

set(NETWORK_SOURCES
    src/network/NetworkSystem.cpp
    src/network/EntityReplication.cpp
)

set(GAMEPLAY_SOURCES
    src/gameplay/Player.cpp
    src/gameplay/Weapon.cpp
    src/gameplay/Vehicle.cpp
    src/gameplay/BattleSystem.cpp
    src/gameplay/WeatherSystem.cpp
)

set(ANIMATION_SOURCES
    src/animation/AnimationSystem.cpp
    src/animation/InverseKinematics.cpp
)

# ── Engine library ─────────────────────────────────────────────────────────────
add_library(ShooterEngineLib STATIC
    ${CORE_SOURCES}
    ${ECS_SOURCES}
    ${RENDERER_SOURCES}
    ${PHYSICS_SOURCES}
    ${AI_SOURCES}
    ${WORLD_SOURCES}
    ${AUDIO_SOURCES}
    ${NETWORK_SOURCES}
    ${GAMEPLAY_SOURCES}
    ${ANIMATION_SOURCES}
)

target_include_directories(ShooterEngineLib PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(ShooterEngineLib PUBLIC Threads::Threads)

if(Vulkan_FOUND AND SHOOTER_ENABLE_VULKAN)
    target_link_libraries(ShooterEngineLib PUBLIC Vulkan::Vulkan)
    target_compile_definitions(ShooterEngineLib PUBLIC SHOOTER_VULKAN_AVAILABLE)
endif()

if(SHOOTER_ENABLE_RAYTRACING)
    target_compile_definitions(ShooterEngineLib PUBLIC SHOOTER_RAYTRACING)
endif()

# ── Main executable ────────────────────────────────────────────────────────────
add_executable(ShooterGame src/main.cpp)
target_link_libraries(ShooterGame PRIVATE ShooterEngineLib)

# ── Editor executable ─────────────────────────────────────────────────────────
if(SHOOTER_BUILD_EDITOR)
    add_executable(ShooterEditor src/editor/EditorMain.cpp)
    target_link_libraries(ShooterEditor PRIVATE ShooterEngineLib)
endif()

# ── Unit tests ─────────────────────────────────────────────────────────────────
if(SHOOTER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
