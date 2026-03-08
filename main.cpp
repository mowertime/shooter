// ---------------------------------------------------------------------------
// main.cpp — Entry point for the Shooter Engine prototype.
//
// Demonstrates:
//  • Engine initialization with a 200 km² world and 500 AI units
//  • Spawning a player, a tank, and several AI squads
//  • Running the engine loop for a few frames
//  • Firing a rifle round and tracing its ballistic trajectory
// ---------------------------------------------------------------------------

#include "engine/Engine.h"
#include "gameplay/Player.h"
#include "gameplay/Weapon.h"
#include "gameplay/Vehicle.h"
#include "gameplay/BattleSystem.h"

#include <iostream>
#include <memory>

int main() {
    using namespace shooter;

    // -----------------------------------------------------------------------
    // 1. Configure the engine
    // -----------------------------------------------------------------------
    EngineConfig cfg;
    cfg.windowTitle         = "Shooter Engine Prototype";
    cfg.windowWidth         = 800;
    cfg.windowHeight        = 600;
    cfg.rendererAPI         = RendererAPI::Vulkan;   // falls back to stub if no SDK
    cfg.enableValidationLayers = true;

    cfg.worldSizeKm2        = 200.f;  // 200 km² ≈ 14.1 × 14.1 km
    cfg.chunkSizeMetres     = 512.f;
    cfg.worldSeed           = 42;

    cfg.maxAIUnits          = 500;
    cfg.enablePhysics       = true;
    cfg.enableNetworking    = false;  // standalone single-player prototype
    cfg.numWorkerThreads    = 0;      // auto-detect

    // -----------------------------------------------------------------------
    // 2. Initialize the engine
    // -----------------------------------------------------------------------
    Engine engine;
    if (!engine.initialize(cfg)) {
        std::cerr << "Fatal: Engine initialization failed.\n";
        return 1;
    }

    // -----------------------------------------------------------------------
    // 3. Spawn a player at world centre
    // -----------------------------------------------------------------------
    PlayerController player;
    f32 worldCentre = std::sqrt(cfg.worldSizeKm2) * 1000.f / 2.f; // km² → metres, half-side
    Vec3 playerSpawn = { worldCentre, 0.f, worldCentre };
    player.initialize(*engine.getWorld(), playerSpawn);

    // -----------------------------------------------------------------------
    // 4. Equip the player with an assault rifle
    // -----------------------------------------------------------------------
    WeaponSystem rifle;
    {
        WeaponStats m4;
        m4.type            = WeaponType::AssaultRifle;
        m4.name            = "M4A1";
        m4.muzzleVelocity  = 910.f;    // m/s
        m4.bulletMass      = 0.004f;   // 4 g (5.56×45 mm)
        m4.bulletDrag      = 0.28f;
        m4.bulletCrossSection = 2.4e-5f;
        m4.damage          = 35.f;
        m4.rpm             = 800.f;
        m4.magazineSize    = 30;
        m4.reserveAmmo     = 120;
        m4.reloadTime      = 2.4f;
        m4.penetration     = { 10.f, 450.f };
        rifle.equip(m4);

        // Attach a suppressor and a 4× ACOG scope.
        WeaponAttachment suppressor;
        suppressor.slot               = AttachmentSlot::Muzzle;
        suppressor.name               = "SilencerCo SOCOM";
        suppressor.muzzleVelocityAdd  = -30.f;
        suppressor.recoilMultiplier   = 0.9f;
        suppressor.suppressesFire     = true;
        rifle.addAttachment(suppressor);

        WeaponAttachment acog;
        acog.slot          = AttachmentSlot::Optic;
        acog.name          = "ACOG 4×32";
        acog.magnification = 4.f;
        acog.spreadMultiplier = 0.7f;
        rifle.addAttachment(acog);
    }

    // -----------------------------------------------------------------------
    // 5. Spawn a tank
    // -----------------------------------------------------------------------
    VehicleSystem vehicleSystem;
    vehicleSystem.initialize();

    VehicleDesc m1a2;
    m1a2.type  = VehicleType::Tank;
    m1a2.name  = "M1A2 Abrams";
    m1a2.physicsConfig.mass         = 62000.f;  // kg
    m1a2.physicsConfig.engineTorque = 4500.f;   // N·m (approx AGT-1500)
    m1a2.physicsConfig.maxSpeed     = 16.7f;    // m/s (~60 km/h road)
    m1a2.physicsConfig.wheelRadius  = 0.5f;
    m1a2.armourThicknessFront = 900.f;          // mm RHA equivalent (Chobham)
    m1a2.armourThicknessSide  = 250.f;
    m1a2.armourThicknessRear  = 80.f;
    m1a2.mainGunCalibremm     = 120.f;
    m1a2.maxOccupants         = 4;

    Vec3 tankSpawn = { worldCentre + 50.f, 0.f, worldCentre };
    EntityID tankID = vehicleSystem.spawnVehicle(*engine.getWorld(), m1a2, tankSpawn);

    // -----------------------------------------------------------------------
    // 6. Set up the battle system
    // -----------------------------------------------------------------------
    BattleSystem battle;
    battle.initialize(engine.getWorld(), engine.getAI(), &vehicleSystem);

    // Add a capture point in the centre of the map.
    BattleObjective centreCap;
    centreCap.name     = "Alpha";
    centreCap.type     = ObjectiveType::CapturePoint;
    centreCap.position = { worldCentre, 0.f, worldCentre };
    centreCap.captureRadius = 75.f;
    u32 capID = battle.addObjective(centreCap);
    (void)capID;

    // Register reinforcement waves for both teams.
    ReinforcementWave blueWave;
    blueWave.teamID           = 1;
    blueWave.squadCount       = 5;
    blueWave.soldiersPerSquad = 8;
    blueWave.spawnZone        = { worldCentre - 2000.f, 0.f, worldCentre };
    blueWave.cooldownSeconds  = 60.f;
    battle.addReinforcementWave(blueWave);

    ReinforcementWave redWave;
    redWave.teamID            = 2;
    redWave.squadCount        = 5;
    redWave.soldiersPerSquad  = 8;
    redWave.spawnZone         = { worldCentre + 2000.f, 0.f, worldCentre };
    redWave.cooldownSeconds   = 60.f;
    battle.addReinforcementWave(redWave);

    battle.setOnObjectiveCaptured([](u32 id, u32 team) {
        std::cout << "[Game] Objective " << id
                  << " captured by team " << team << "!\n";
    });

    // -----------------------------------------------------------------------
    // 7. Spawn some initial AI squads
    // -----------------------------------------------------------------------
    {
        AIManager* ai = engine.getAI();

        // Blue force (team 1): attack squad near player
        u32 blueSquadID = ai->createSquad("Blue-1", 1);
        for (u32 i = 0; i < 6; ++i) {
            Vec3 pos = { worldCentre - 100.f + i * 10.f, 0.f, worldCentre };
            EntityID aid = ai->spawnAgent(*engine.getWorld(), pos, 1);
            ai->assignToSquad(aid, blueSquadID);
        }
        ai->issueSquadOrder(blueSquadID, Tactic::Attack,
                            centreCap.position);

        // Red force (team 2): defend squad on objective
        u32 redSquadID = ai->createSquad("Red-1", 2);
        for (u32 i = 0; i < 6; ++i) {
            Vec3 pos = { worldCentre + 80.f + i * 10.f, 0.f, worldCentre };
            EntityID aid = ai->spawnAgent(*engine.getWorld(), pos, 2);
            ai->assignToSquad(aid, redSquadID);
        }
        ai->issueSquadOrder(redSquadID, Tactic::Defend, centreCap.position);

        std::cout << "[Game] Spawned " << ai->agentCount()
                  << " AI agents in " << ai->squadCount() << " squads\n";
    }

    // -----------------------------------------------------------------------
    // 8. Demonstrate ballistic trajectory simulation
    // -----------------------------------------------------------------------
    {
        PhysicsWorld ballistics;
        ballistics.initialize();

        ProjectileDesc sniper;
        sniper.origin         = playerSpawn;
        sniper.direction      = { 1.f, 0.02f, 0.f }; // slight upward angle
        sniper.direction      = sniper.direction.normalised();
        sniper.muzzleVelocity = 890.f;      // 7.62×51mm M118LR
        sniper.mass           = 0.01154f;   // 178 gr
        sniper.dragCoefficient= 0.21f;      // Berger VLD
        sniper.crossSection   = 4.52e-5f;   // 7.62 mm
        sniper.simulationTime = 3.f;        // 3 seconds of flight
        sniper.wind           = { 3.f, 0.f, 0.f }; // 3 m/s crosswind

        auto trajectory = ballistics.simulateProjectile(sniper);
        std::cout << "\n[Ballistics] 7.62mm trajectory (" 
                  << trajectory.size() << " samples):\n";
        for (size_t i = 0; i < trajectory.size() && i < 5; ++i) {
            const auto& pt = trajectory[i];
            std::cout << "  t=" << pt.time
                      << " s  pos=(" << pt.position.x
                      << ", "        << pt.position.y
                      << ", "        << pt.position.z << ")\n";
        }
        if (trajectory.size() > 5) {
            const auto& last = trajectory.back();
            std::cout << "  ... t=" << last.time
                      << " s  final=(" << last.position.x
                      << ", "          << last.position.y
                      << ", "          << last.position.z << ")\n";
        }
        ballistics.shutdown();
    }

    // -----------------------------------------------------------------------
    // 9. Run the engine main loop
    // -----------------------------------------------------------------------
    std::cout << "\n[Game] Starting main loop...\n";
    engine.run();

    // -----------------------------------------------------------------------
    // 10. Cleanup
    // -----------------------------------------------------------------------
    player.destroy(*engine.getWorld());
    vehicleSystem.despawnVehicle(*engine.getWorld(), tankID);
    battle.shutdown();
    vehicleSystem.shutdown();
    engine.shutdown();

    std::cout << "\n[Game] Prototype finished cleanly.\n";
    return 0;
}
