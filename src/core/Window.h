#pragma once

#include "core/Platform.h"
#include <string>
#include <functional>

namespace shooter {

/// Window creation parameters.
struct WindowDesc {
    std::string title   = "Shooter Engine";
    u32         width   = 1920;
    u32         height  = 1080;
    bool        fullscreen = false;
    bool        vsync      = true;
};

/// Abstract window that wraps OS-specific windowing.
///
/// The window owns the native handle and fires event callbacks
/// (resize, close, key/mouse input) set by the caller.
class Window {
public:
    using ResizeCallback = std::function<void(u32 width, u32 height)>;
    using CloseCallback  = std::function<void()>;

    /// Create and show a window according to \p desc.
    /// Returns nullptr on failure.
    static Window* create(const WindowDesc& desc);

    virtual ~Window() = default;

    /// Poll OS messages; returns false when the user requested close.
    virtual bool pollEvents() = 0;

    /// Swap front/back buffers (for non-Vulkan surface paths).
    virtual void swapBuffers() = 0;

    virtual u32         getWidth()  const = 0;
    virtual u32         getHeight() const = 0;
    virtual bool        isMinimised() const = 0;
    virtual const char* getTitle()  const = 0;

    /// Return the platform-native window handle (HWND, xcb_window_t, …).
    virtual void*       getNativeHandle() const = 0;

    void setResizeCallback(ResizeCallback cb) { m_resizeCb = std::move(cb); }
    void setCloseCallback (CloseCallback  cb) { m_closeCb  = std::move(cb); }

protected:
    ResizeCallback m_resizeCb;
    CloseCallback  m_closeCb;
};

} // namespace shooter
