#pragma once

#include "core/Platform.h"
#include <atomic>
#include <functional>
#include <memory>
#include <string>

namespace shooter {

/// Job priority levels for the thread pool.
enum class JobPriority : u8 {
    Critical = 0, ///< Render-thread, physics step
    High     = 1, ///< AI update, streaming
    Normal   = 2, ///< Asset decompression, procedural gen
    Low      = 3, ///< Background tasks
};

using JobFunc = std::function<void()>;

/// Lock-free multi-producer / multi-consumer thread pool.
///
/// Work is divided across \p workerCount threads.  A work-stealing
/// strategy is used to keep all cores busy during uneven loads.
class ThreadPool {
public:
    explicit ThreadPool(u32 workerCount = 0); ///< 0 → hardware_concurrency - 1
    ~ThreadPool();

    /// Submit a job and return a completion token (future-like).
    void submit(JobFunc job, JobPriority priority = JobPriority::Normal);

    /// Block until all pending jobs have finished.
    void waitIdle();

    u32  workerCount() const { return m_workerCount; }

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    u32 m_workerCount{0};
};

/// Scoped spinlock for short critical sections.
class SpinLock {
public:
    void lock();
    void unlock();
    bool tryLock();
private:
    std::atomic_flag m_flag = ATOMIC_FLAG_INIT;
};

/// Fiber-compatible job counter.  Callers wait on this until the
/// associated batch of jobs reaches zero.
class JobCounter {
public:
    explicit JobCounter(u32 initial = 0);
    void     decrement();
    void     waitUntilZero();
    u32      value() const;
private:
    std::atomic<u32> m_count;
};

} // namespace shooter
