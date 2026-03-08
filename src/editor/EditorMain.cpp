/**
 * ShooterEditor – World editor entry point.
 *
 * Minimal stub that can be expanded into a full IMGUI-based editor
 * supporting: terrain sculpting, biome painting, object placement,
 * mission scripting, and AI debugging.
 */

#include "core/Platform.h"
#include "core/Logger.h"
#include "core/Window.h"
#include "world/WorldSystem.h"
#include "renderer/Renderer.h"
#include "ai/CoverSystem.h"

using namespace shooter;

int main() {
    PlatformInfo info{};
    platformInit(info);
    LOG_INFO("Editor", "Starting ShooterEditor on %s", info.name);

    WindowDesc wd;
    wd.title  = "ShooterEditor";
    wd.width  = 1280;
    wd.height = 720;
    std::unique_ptr<Window> window(Window::create(wd));

    RendererDesc rd;
    rd.backend = RendererBackend::Stub;
    rd.window  = window.get();
    auto renderer = Renderer::create(rd);

    WorldDesc wld;
    wld.widthKm  = 10.0f; // Small preview world for editing
    wld.heightKm = 10.0f;
    wld.seed     = 99;
    WorldSystem world(wld);
    world.init();

    LOG_INFO("Editor", "World ready – editor loop running (stub)");

    // Editor loop stub: in a full build this drives an ImGui interface.
    constexpr int MAX_FRAMES = 10;
    for (int i = 0; i < MAX_FRAMES; ++i) {
        window->pollEvents();
        renderer->beginFrame();
        renderer->endFrame();
    }

    world.shutdown();
    renderer->shutdown();
    platformShutdown();
    LOG_INFO("Editor", "Editor exited cleanly");
    return 0;
}
