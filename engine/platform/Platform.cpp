// ---------------------------------------------------------------------------
// Platform.cpp — Implementations of FileSystem, Window, and Threading.
// ---------------------------------------------------------------------------

#include "engine/platform/Platform.h"
#include "engine/platform/Window.h"
#include "engine/platform/FileSystem.h"
#include "engine/platform/Threading.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>

// ===========================================================================
// FileSystem implementation
// ===========================================================================
namespace shooter {

std::vector<uint8_t> FileSystem::readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[FileSystem] Failed to open: " << path << "\n";
        return {};
    }
    auto size = file.tellg();
    file.seekg(0);
    std::vector<uint8_t> buf(static_cast<size_t>(size));
    file.read(reinterpret_cast<char*>(buf.data()), size);
    return buf;
}

bool FileSystem::writeFile(const std::string& path,
                           const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "[FileSystem] Failed to write: " << path << "\n";
        return false;
    }
    file.write(reinterpret_cast<const char*>(data.data()),
               static_cast<std::streamsize>(data.size()));
    return file.good();
}

bool FileSystem::exists(const std::string& path) {
    return std::filesystem::exists(path);
}

std::vector<std::string> FileSystem::listFiles(const std::string& dirPath) {
    std::vector<std::string> results;
    if (!std::filesystem::is_directory(dirPath)) return results;
    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
        results.push_back(entry.path().string());
    }
    return results;
}

// ===========================================================================
// Stub Window — no actual OS window; just logs to stdout.
// A real port would create a Win32/X11/Wayland window here.
// ===========================================================================
class StubWindow : public IWindow {
public:
    bool create(const WindowDesc& desc) override {
        m_desc = desc;
        std::cout << "[Window] Created stub window \""
                  << desc.title << "\" "
                  << desc.width << "x" << desc.height
                  << (desc.fullscreen ? " (fullscreen)" : "") << "\n";
        return true;
    }

    void destroy() override {
        std::cout << "[Window] Destroyed stub window\n";
    }

    void pollEvents() override {
        // TODO: pump Win32/X11 event queue here
    }

    bool shouldClose() const override { return m_shouldClose; }

    u32 getWidth()  const override { return m_desc.width; }
    u32 getHeight() const override { return m_desc.height; }

    void* getNativeHandle() const override { return nullptr; }

private:
    WindowDesc m_desc;
    bool       m_shouldClose{false};
};

std::unique_ptr<IWindow> WindowFactory::create() {
    return std::make_unique<StubWindow>();
}

// ===========================================================================
// ThreadPool implementation
// ===========================================================================
ThreadPool::ThreadPool(u32 numThreads) {
    if (numThreads == 0)
        numThreads = std::max(1u, std::thread::hardware_concurrency() - 1);

    m_workers.reserve(numThreads);
    for (u32 i = 0; i < numThreads; ++i) {
        m_workers.emplace_back(&ThreadPool::workerLoop, this);
    }
    std::cout << "[ThreadPool] Started " << numThreads << " workers\n";
}

ThreadPool::~ThreadPool() {
    {
        std::scoped_lock lock(m_mutex);
        m_stopping = true;
    }
    m_cv.notify_all();
    for (auto& w : m_workers) {
        if (w.joinable()) w.join();
    }
}

void ThreadPool::workerLoop() {
    for (;;) {
        std::function<void()> task;
        {
            std::unique_lock lock(m_mutex);
            m_cv.wait(lock, [this]{ return m_stopping || !m_tasks.empty(); });
            if (m_stopping && m_tasks.empty()) return;
            task = std::move(m_tasks.front());
            m_tasks.pop();
            m_activeTasks.fetch_add(1, std::memory_order_relaxed);
        }
        task();
        {
            std::scoped_lock lock(m_mutex);
            m_activeTasks.fetch_sub(1, std::memory_order_relaxed);
        }
        m_cvDone.notify_all();
    }
}

void ThreadPool::waitAll() {
    std::unique_lock lock(m_mutex);
    m_cvDone.wait(lock, [this]{
        return m_tasks.empty() &&
               m_activeTasks.load(std::memory_order_relaxed) == 0;
    });
}

// ===========================================================================
// JobSystem implementation
// ===========================================================================
JobSystem::JobSystem(u32 numWorkers)
    : m_pool(std::make_unique<ThreadPool>(numWorkers)) {}

JobID JobSystem::enqueue(std::string name,
                         std::function<void()> work,
                         std::vector<JobID> deps) {
    auto job       = std::make_unique<Job>();
    job->id        = m_nextID.fetch_add(1, std::memory_order_relaxed);
    job->name      = std::move(name);
    job->work      = std::move(work);
    job->dependencies = std::move(deps);
    job->pendingDeps.store(static_cast<i32>(job->dependencies.size()),
                           std::memory_order_relaxed);
    JobID id = job->id;
    {
        std::scoped_lock lock(m_jobMutex);
        m_pendingJobs.push_back(std::move(job));
    }
    return id;
}

void JobSystem::dispatch() {
    // Simple O(n²) dependency resolution for prototype.
    // A production system uses a DAG with sorted topological order.
    std::scoped_lock lock(m_jobMutex);
    for (auto& job : m_pendingJobs) {
        if (job->pendingDeps.load(std::memory_order_relaxed) == 0) {
            m_inFlight.fetch_add(1, std::memory_order_relaxed);
            Job* rawJob = job.get();
            m_pool->submit([this, rawJob]() {
                rawJob->work();
                m_inFlight.fetch_sub(1, std::memory_order_relaxed);
                std::unique_lock lk(m_doneMutex);
                m_doneCv.notify_all();
            });
        }
    }
    m_pendingJobs.clear();
    waitIdle();
}

void JobSystem::waitIdle() {
    std::unique_lock lock(m_doneMutex);
    m_doneCv.wait(lock, [this]{
        return m_inFlight.load(std::memory_order_relaxed) == 0;
    });
}

} // namespace shooter
