#include "core/FileSystem.h"
#include "core/Logger.h"

#include <filesystem>
#include <fstream>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <utility>

namespace fs = std::filesystem;

namespace shooter {

struct FileSystem::Impl {
    // Virtual-to-host path mount table.
    struct Mount {
        std::string virtualPrefix;
        std::string hostPath;
    };
    std::vector<Mount> mounts;
    mutable std::mutex mountMutex;

    // Async I/O thread.
    struct AsyncJob {
        std::string          path;
        FileReadCallback     callback;
    };
    std::queue<AsyncJob>    jobQueue;
    std::mutex              queueMutex;
    std::condition_variable queueCv;
    std::atomic<bool>       running{true};
    std::thread             ioThread;

    Impl() {
        ioThread = std::thread([this] {
            while (running.load()) {
                AsyncJob job;
                {
                    std::unique_lock lock(queueMutex);
                    queueCv.wait(lock, [this] {
                        return !jobQueue.empty() || !running.load();
                    });
                    if (!running.load() && jobQueue.empty()) break;
                    job = std::move(jobQueue.front());
                    jobQueue.pop();
                }
                std::vector<u8> data;
                bool ok = FileSystem::get().readFile(job.path, data);
                job.callback(std::move(data), ok);
            }
        });
    }

    ~Impl() {
        {
            std::lock_guard lock(queueMutex);
            running = false;
        }
        queueCv.notify_all();
        if (ioThread.joinable()) ioThread.join();
    }

    /// Resolve a virtual path to an absolute host path.
    std::string resolve(const std::string& virtualPath) const {
        std::lock_guard lock(mountMutex);
        for (const auto& m : mounts) {
            if (virtualPath.rfind(m.virtualPrefix, 0) == 0) {
                std::string rel = virtualPath.substr(m.virtualPrefix.size());
                return m.hostPath + rel;
            }
        }
        return virtualPath; // fall-through: treat as literal host path
    }
};

static FileSystem s_instance;

FileSystem& FileSystem::get() { return s_instance; }

FileSystem::FileSystem()  : m_impl(new Impl()) {}
FileSystem::~FileSystem() { delete m_impl; }

bool FileSystem::mount(const std::string& hostPath,
                       const std::string& virtualPrefix) {
    std::lock_guard lock(m_impl->mountMutex);
    m_impl->mounts.push_back({virtualPrefix, hostPath});
    LOG_INFO("FileSystem", "Mounted '{}' -> '{}'", virtualPrefix, hostPath);
    return true;
}

bool FileSystem::readFile(const std::string& virtualPath,
                          std::vector<u8>& outData) {
    std::string host = m_impl->resolve(virtualPath);
    std::ifstream f(host, std::ios::binary | std::ios::ate);
    if (!f.is_open()) {
        LOG_WARN("FileSystem", "Could not open '{}'", host);
        return false;
    }
    auto size = static_cast<std::streamsize>(f.tellg());
    f.seekg(0, std::ios::beg);
    outData.resize(static_cast<size_t>(size));
    f.read(reinterpret_cast<char*>(outData.data()), size);
    return f.good();
}

bool FileSystem::writeFile(const std::string& virtualPath,
                           const void* data, u64 size) {
    std::string host = m_impl->resolve(virtualPath);
    // Ensure parent directories exist.
    fs::create_directories(fs::path(host).parent_path());
    std::ofstream f(host, std::ios::binary | std::ios::trunc);
    if (!f.is_open()) return false;
    f.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    return f.good();
}

void FileSystem::readFileAsync(const std::string& virtualPath,
                               FileReadCallback callback) {
    std::lock_guard lock(m_impl->queueMutex);
    m_impl->jobQueue.push({virtualPath, std::move(callback)});
    m_impl->queueCv.notify_one();
}

bool FileSystem::fileExists(const std::string& virtualPath) const {
    return fs::exists(m_impl->resolve(virtualPath));
}

u64 FileSystem::fileSize(const std::string& virtualPath) const {
    std::error_code ec;
    auto sz = fs::file_size(m_impl->resolve(virtualPath), ec);
    return ec ? 0 : static_cast<u64>(sz);
}

void FileSystem::flushAsyncQueue() {
    // Simple spin-wait; production code would use a semaphore/fence.
    while (true) {
        std::lock_guard lock(m_impl->queueMutex);
        if (m_impl->jobQueue.empty()) break;
    }
}

} // namespace shooter
