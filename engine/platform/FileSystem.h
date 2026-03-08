#pragma once

// ---------------------------------------------------------------------------
// FileSystem.h — Abstraction over file I/O.
// ---------------------------------------------------------------------------

#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>

namespace shooter {

// ---------------------------------------------------------------------------
// IFileSystem — pure interface; makes unit-testing straightforward by allowing
// a mock implementation to be injected.
// ---------------------------------------------------------------------------
class IFileSystem {
public:
    virtual ~IFileSystem() = default;

    /// Read entire file into a byte buffer. Returns empty vector on failure.
    virtual std::vector<uint8_t> readFile(const std::string& path) = 0;

    /// Write bytes to file, creating or truncating as needed.
    /// Returns true on success.
    virtual bool writeFile(const std::string& path,
                           const std::vector<uint8_t>& data) = 0;

    /// Returns true if the file or directory exists.
    virtual bool exists(const std::string& path) = 0;

    /// List all direct children of a directory path.
    virtual std::vector<std::string> listFiles(const std::string& dirPath) = 0;
};

// ---------------------------------------------------------------------------
// FileSystem — concrete implementation backed by std::filesystem.
// ---------------------------------------------------------------------------
class FileSystem : public IFileSystem {
public:
    std::vector<uint8_t> readFile(const std::string& path) override;
    bool writeFile(const std::string& path,
                   const std::vector<uint8_t>& data) override;
    bool exists(const std::string& path) override;
    std::vector<std::string> listFiles(const std::string& dirPath) override;
};

} // namespace shooter
