#pragma once

// ---------------------------------------------------------------------------
// Window.h — Platform-agnostic window abstraction.
// ---------------------------------------------------------------------------

#include <string>
#include <memory>
#include "Platform.h"

namespace shooter {

/// Parameters used to create a window.
struct WindowDesc {
    std::string title     = "Shooter Engine";
    u32         width     = 1920;
    u32         height    = 1080;
    bool        fullscreen = false;
};

// ---------------------------------------------------------------------------
// IWindow — pure interface representing a native OS window.
// Concrete implementations live per-platform (Win32, X11, Cocoa, …).
// ---------------------------------------------------------------------------
class IWindow {
public:
    virtual ~IWindow() = default;

    /// Create and show the window.
    virtual bool create(const WindowDesc& desc) = 0;

    /// Destroy the window and release native resources.
    virtual void destroy() = 0;

    /// Pump OS message queue; must be called every frame.
    virtual void pollEvents() = 0;

    /// Returns true once the user has requested the window be closed.
    [[nodiscard]] virtual bool shouldClose() const = 0;

    [[nodiscard]] virtual u32 getWidth()  const = 0;
    [[nodiscard]] virtual u32 getHeight() const = 0;

    /// Returns the underlying native handle (HWND on Windows, Display* on X11, etc.).
    [[nodiscard]] virtual void* getNativeHandle() const = 0;
};

// ---------------------------------------------------------------------------
// WindowFactory — creates the appropriate IWindow for the current platform.
// ---------------------------------------------------------------------------
class WindowFactory {
public:
    /// Allocate and return a platform window. Caller owns the object.
    static std::unique_ptr<IWindow> create();
};

} // namespace shooter
