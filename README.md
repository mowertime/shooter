# Shooter – Next-Generation Large-Scale Open-World Combat Engine

A **C++20 engine prototype** built from scratch to support gameplay at the scale of Battlefield, ARMA, and GTA. Uses a **Vulkan / DirectX 12** rendering backend (stub-safe when no GPU API is available), **data-oriented design**, and a **multithreaded architecture**.

---

## Feature Summary

| Domain | Features |
|---|---|
| **World** | 200+ km² maps, procedural terrain (fractal noise + biomes), asynchronous tile streaming, procedural city generation |
| **Renderer** | Vulkan / DX12 backend abstraction, deferred + forward hybrid, PBR, HDR, volumetric lighting, declarative render graph, GPU buffer management |
| **ECS** | Versioned entity handles, dense component arrays, pluggable system pipeline |
| **Physics** | Rigid body (gravity, restitution), vehicle dynamics, aircraft physics (lift/drag), ballistic projectile simulation with drag + wind, destruction system |
| **AI** | Composable behaviour trees, per-agent blackboard, AI controller pool, squad manager with formations, perception system, spatial cover database |
| **Gameplay** | Advanced FPS player (vault, crouch, prone, lean), modular weapon system with real ballistic profiles (AK-47, M4A1, M82 Barrett, Glock 17, M249, RPG-7), vehicle/aircraft instances, dynamic front lines, weather + day/night cycle |
| **Audio** | Spatial audio (HRTF-ready), equal-power panning, inverse-square attenuation, async emitter management |
| **Networking** | Offline / server / client role abstraction, entity snapshot replication, prediction/rollback stubs |
| **Animation** | Skeletal animation with clip blending, analytical two-bone IK, FABRIK multi-bone IK solver |
| **Core** | Platform abstraction (Linux / Windows / macOS), printf-based logger, aligned memory allocators, async file system, work-stealing thread pool, spin lock, job counter |

---

## Building

### Requirements
- CMake >= 3.20
- C++20 compiler (GCC 13, Clang 16, MSVC 2022)
- (Optional) Vulkan SDK for GPU rendering

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### CMake options

| Option | Default | Description |
|---|---|---|
| `SHOOTER_ENABLE_VULKAN` | ON | Link Vulkan SDK if found |
| `SHOOTER_ENABLE_DX12` | OFF | Enable DirectX 12 backend (Windows) |
| `SHOOTER_ENABLE_RAYTRACING` | OFF | Enable VK_KHR_ray_tracing_pipeline |
| `SHOOTER_BUILD_TESTS` | ON | Build unit test binary |
| `SHOOTER_BUILD_EDITOR` | ON | Build world editor binary |

---

## Running

```bash
./build/ShooterGame          # Main engine demo (180 frames, clean shutdown)
./build/ShooterEditor        # World editor stub
./build/tests/ShooterTests   # Unit tests (710 assertions)
```

---

## Architecture

```
shooter/
├── src/
│   ├── core/          Platform, Window, FileSystem, Threading, Memory, Logger
│   ├── ecs/           Entity, Component, System, EntityRegistry
│   ├── renderer/      Renderer (abstract), VulkanRenderer, RenderGraph, GpuBuffer
│   ├── physics/       PhysicsWorld, RigidBody, VehiclePhysics, ProjectileSimulator, DestructionSystem
│   ├── ai/            BehaviorTree, AIController, SquadManager, PerceptionSystem, CoverSystem
│   ├── world/         WorldSystem, Terrain, WorldStreamer, BiomeSystem, ProceduralCity
│   ├── audio/         AudioSystem, SpatialAudio
│   ├── network/       NetworkSystem, EntityReplication
│   ├── gameplay/      Player, Weapon, Vehicle, BattleSystem, WeatherSystem
│   ├── animation/     AnimationSystem, InverseKinematics
│   ├── editor/        EditorMain (world editor entry point)
│   └── main.cpp       Engine entry point and game loop
└── tests/             Unit tests (ECS, Physics, AI, World, Audio, Ballistics, Threading)
```

### Key design decisions

- **ECS with dense packed arrays** – component storage is a flat vector per type, enabling cache-friendly iteration and O(1) add/remove via swap-with-last.
- **Work-stealing thread pool** – all heavy work (AI, streaming, physics substeps) submits jobs at configurable priority levels.
- **Render graph** – passes declare their resources; the graph resolves data dependencies, inserts barriers, and culls unused passes automatically.
- **Ballistic simulation** – projectiles use Euler integration with Cd x area drag and wind vectors; per-weapon `armorPenetration` values drive damage falloff through armoured targets.
- **Behaviour trees via lambdas** – `BtAction` and `BtCondition` take `std::function` closures so new behaviours can be composed at runtime without subclassing.
- **Versioned entity handles** – the upper 32 bits of an entity id store a generation counter, making dangling-handle detection O(1) with no extra bookkeeping.

---

## Roadmap (production path)

1. Integrate real Vulkan swap-chain, G-buffer, and PBR pipeline.
2. Replace stub physics with a production solver (e.g., Jolt Physics).
3. Add NavMesh generation and pathfinding for AI movement.
4. Implement full asset pipeline (GLTF loader, texture streaming, shader compilation).
5. Add network transport layer (reliable UDP, delta compression).
6. Build out the ImGui-based world editor.
7. Implement full skeletal animation pipeline (GPU skinning, blend trees, ragdoll).