# Shooter — Large-Scale Open-World Combat Simulation Engine

A minimal C++20 engine prototype designed for Battlefield/ARMA/GTA-scale combat simulations.

## Features (Prototype)

| System | Status |
|---|---|
| Platform (window, filesystem, threading, memory) | ✅ Prototype |
| Renderer (Vulkan/DX12 abstraction, deferred + forward hybrid) | ✅ Prototype |
| ECS (archetype-based, cache-friendly) | ✅ Prototype |
| Physics (RK4 ballistics, rigid bodies, vehicle dynamics) | ✅ Prototype |
| World (fBm terrain, async streaming, 200 km²) | ✅ Prototype |
| AI (behavior trees, squad tactics, LOD scheduling) | ✅ Prototype |
| Animation (skeletal, IK, ragdoll) | ✅ Prototype |
| Audio (3D spatial, battlefield acoustics) | ✅ Prototype |
| Networking (entity replication, prediction/rollback) | ✅ Prototype |
| Gameplay (FPS player, modular weapons, vehicles, battle system) | ✅ Prototype |

## Building

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/Shooter
```

Requires CMake 3.20+ and a C++20 compiler. Vulkan SDK is optional; the renderer falls back to a stub implementation automatically.

## Structure

```
engine/
  platform/    Platform abstraction (window, filesystem, threading, memory)
  renderer/    Vulkan/DX12 renderer (deferred + forward hybrid, PBR)
  ecs/         Entity Component System (archetype-based, multithreaded)
  physics/     Physics (rigid/soft bodies, vehicles, projectile simulation)
  world/       World system (terrain, async streaming, biomes)
  ai/          AI (behavior trees, squad coordination, cover system)
  animation/   Skeletal animation, IK, ragdoll
  audio/       3D spatial audio, battlefield acoustics
  networking/  Client/server, entity replication, prediction + rollback
gameplay/
  Player        FPS movement (vault, climb, prone, lean)
  Weapon        Modular weapons, ballistics, armor penetration
  Vehicle       Tanks, APCs, helicopters, jets, boats
  BattleSystem  Dynamic front lines, objectives, reinforcements
main.cpp        Entry point / demo
```