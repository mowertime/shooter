#pragma once

#include "core/Platform.h"
#include <string>
#include <vector>
#include <functional>

namespace shooter {

/// Asynchronous file-read completion callback.
using FileReadCallback = std::function<void(std::vector<u8> data, bool success)>;

/// Abstract file-system interface.
///
/// Supports both synchronous and asynchronous reads so that the world
/// streaming system can queue I/O without blocking the render thread.
class FileSystem {
public:
    static FileSystem& get();

    /// Mount a directory or archive file as a virtual path prefix.
    /// e.g. mount("/game/data", "data") makes data/terrain/… accessible.
    bool mount(const std::string& hostPath, const std::string& virtualPrefix);

    /// Synchronous read – blocks the calling thread.
    bool readFile(const std::string& virtualPath, std::vector<u8>& outData);

    /// Write a file (used by the editor and save-game system).
    bool writeFile(const std::string& virtualPath, const void* data, u64 size);

    /// Asynchronous read – posts work to the I/O thread pool.
    void readFileAsync(const std::string& virtualPath, FileReadCallback callback);

    /// Returns true if the given virtual path exists.
    bool fileExists(const std::string& virtualPath) const;

    /// Returns the file size in bytes, or 0 if not found.
    u64 fileSize(const std::string& virtualPath) const;

    /// Block until all pending async reads are complete.
    void flushAsyncQueue();

    FileSystem();
    ~FileSystem();

private:

    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace shooter
