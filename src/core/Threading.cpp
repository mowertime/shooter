#include "core/Threading.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace shooter {

// ── ThreadPool ────────────────────────────────────────────────────────────────

struct ThreadPool::Impl {
    struct Job {
        JobFunc      func;
        JobPriority  priority;
        bool operator<(const Job& o) const {
            return static_cast<u8>(priority) > static_cast<u8>(o.priority);
        }
    };

    std::priority_queue<Job>    queue;
    std::mutex                  queueMutex;
    std::condition_variable     queueCv;
    std::atomic<bool>           running{true};
    std::atomic<u32>            activeTasks{0};
    std::condition_variable     idleCv;
    std::mutex                  idleMutex;
    std::vector<std::thread>    workers;

    void workerLoop() {
        while (running.load(std::memory_order_relaxed)) {
            Job job;
            {
                std::unique_lock lock(queueMutex);
                queueCv.wait(lock, [this] {
                    return !queue.empty() || !running.load();
                });
                if (!running.load() && queue.empty()) return;
                job = std::move(const_cast<Job&>(queue.top()));
                queue.pop();
                activeTasks.fetch_add(1, std::memory_order_relaxed);
            }
            job.func();
            u32 prev = activeTasks.fetch_sub(1, std::memory_order_acq_rel);
            if (prev == 1) {
                std::lock_guard lk(idleMutex);
                idleCv.notify_all();
            }
        }
    }
};

ThreadPool::ThreadPool(u32 workerCount) : m_impl(std::make_unique<Impl>()) {
    if (workerCount == 0) {
        u32 hw = std::thread::hardware_concurrency();
        workerCount = (hw > 1) ? (hw - 1) : 1;
    }
    m_workerCount = workerCount;
    m_impl->workers.reserve(workerCount);
    for (u32 i = 0; i < workerCount; ++i) {
        m_impl->workers.emplace_back([this] { m_impl->workerLoop(); });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard lock(m_impl->queueMutex);
        m_impl->running = false;
    }
    m_impl->queueCv.notify_all();
    for (auto& w : m_impl->workers) {
        if (w.joinable()) w.join();
    }
}

void ThreadPool::submit(JobFunc job, JobPriority priority) {
    {
        std::lock_guard lock(m_impl->queueMutex);
        m_impl->queue.push({std::move(job), priority});
    }
    m_impl->queueCv.notify_one();
}

void ThreadPool::waitIdle() {
    std::unique_lock lock(m_impl->idleMutex);
    m_impl->idleCv.wait(lock, [this] {
        std::lock_guard ql(m_impl->queueMutex);
        return m_impl->queue.empty() &&
               m_impl->activeTasks.load(std::memory_order_acquire) == 0;
    });
}

// ── SpinLock ──────────────────────────────────────────────────────────────────

void SpinLock::lock() {
    while (m_flag.test_and_set(std::memory_order_acquire)) {
        // Busy-wait; a production implementation would use _mm_pause() here.
    }
}

void SpinLock::unlock() {
    m_flag.clear(std::memory_order_release);
}

bool SpinLock::tryLock() {
    return !m_flag.test_and_set(std::memory_order_acquire);
}

// ── JobCounter ────────────────────────────────────────────────────────────────

JobCounter::JobCounter(u32 initial) : m_count(initial) {}

void JobCounter::decrement() {
    m_count.fetch_sub(1, std::memory_order_acq_rel);
}

void JobCounter::waitUntilZero() {
    // Spin-wait; a real engine would yield the fiber instead.
    while (m_count.load(std::memory_order_acquire) != 0) {
        std::this_thread::yield();
    }
}

u32 JobCounter::value() const {
    return m_count.load(std::memory_order_relaxed);
}

} // namespace shooter
