#include "core/Window.h"
#include "core/Logger.h"

namespace shooter {

// ── Stub window (headless / no native windowing library linked) ───────────────
// In a production build this would be replaced with a platform-specific
// implementation (Win32, xcb/xlib, Cocoa) or a GLFW/SDL wrapper.

class StubWindow final : public Window {
public:
    explicit StubWindow(const WindowDesc& desc)
        : m_desc(desc) {}

    bool pollEvents() override {
        // In a real implementation this processes the OS message queue.
        // The stub never requests a close.
        return true;
    }

    void swapBuffers() override {}

    u32         getWidth()        const override { return m_desc.width;  }
    u32         getHeight()       const override { return m_desc.height; }
    bool        isMinimised()     const override { return false; }
    const char* getTitle()        const override { return m_desc.title.c_str(); }
    void*       getNativeHandle() const override { return nullptr; }

private:
    WindowDesc m_desc;
};

Window* Window::create(const WindowDesc& desc) {
    LOG_INFO("Window", "Creating window '{}' {}x{} (fullscreen={})",
             desc.title, desc.width, desc.height, desc.fullscreen);
    return new StubWindow(desc);
}

} // namespace shooter
