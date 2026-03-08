#pragma once

// ---------------------------------------------------------------------------
// Threading.h — Thread pool, job system, and fiber stubs.
// ---------------------------------------------------------------------------

#include <functional>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <future>

#include "Platform.h"

namespace shooter {

// ===========================================================================
// ThreadPool — fixed-size pool of worker threads.
// Submits std::function<void()> tasks from any thread.
// ===========================================================================
class ThreadPool {
public:
    explicit ThreadPool(u32 numThreads = 0);
    ~ThreadPool();

    /// Enqueue a callable and return a future to track completion.
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>
    {
        using RetType = std::invoke_result_t<F, Args...>;
        auto task = std::make_shared<std::packaged_task<RetType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<RetType> result = task->get_future();
        {
            std::scoped_lock lock(m_mutex);
            SE_ASSERT(!m_stopping, "Cannot submit to a stopped ThreadPool");
            m_tasks.emplace([task]{ (*task)(); });
        }
        m_cv.notify_one();
        return result;
    }

    /// Block until all queued tasks have been executed.
    void waitAll();

    [[nodiscard]] u32 threadCount() const { return static_cast<u32>(m_workers.size()); }

private:
    void workerLoop();

    std::vector<std::thread>          m_workers;
    std::queue<std::function<void()>> m_tasks;
    std::mutex                        m_mutex;
    std::condition_variable           m_cv;
    std::condition_variable           m_cvDone;
    std::atomic<u32>                  m_activeTasks{0};
    bool                              m_stopping{false};
};

// ===========================================================================
// Job / JobSystem — dependency-aware job graph executed on a ThreadPool.
//
// Example usage:
//   auto jid = jobSystem.enqueue("UpdateAI", []{ /* ... */ });
//   auto jid2 = jobSystem.enqueue("RenderSync", []{ /* ... */ }, {jid});
//   jobSystem.dispatch();
// ===========================================================================

using JobID = u64;

struct Job {
    JobID                      id{0};
    std::string                name;
    std::function<void()>      work;
    std::vector<JobID>         dependencies;  // jobs that must finish first
    std::atomic<i32>           pendingDeps{0};
};

class JobSystem {
public:
    explicit JobSystem(u32 numWorkers = 0);

    /// Register a job.  Returns a handle that can be used as a dependency.
    JobID enqueue(std::string name,
                  std::function<void()> work,
                  std::vector<JobID> deps = {});

    /// Submit all ready jobs to the underlying ThreadPool and wait for them.
    void dispatch();

    void waitIdle();

private:
    void tryScheduleJob(Job& job);

    std::unique_ptr<ThreadPool>            m_pool;
    std::vector<std::unique_ptr<Job>>      m_pendingJobs;
    std::mutex                             m_jobMutex;
    std::atomic<JobID>                     m_nextID{1};
    std::atomic<i32>                       m_inFlight{0};
    std::condition_variable                m_doneCv;
    std::mutex                             m_doneMutex;
};

// ===========================================================================
// Fiber — cooperative, stack-switching tasks (stub).
//
// A production implementation would use platform fiber APIs
// (CreateFiber/SwitchToFiber on Windows, getcontext/makecontext on POSIX).
// This stub provides the interface so callers can be written now and the
// real implementation plugged in later.
// ===========================================================================
class Fiber {
public:
    using FiberFn = std::function<void()>;

    explicit Fiber(FiberFn fn) : m_fn(std::move(fn)) {}

    /// Resume execution of the fiber.
    void resume() {
        // TODO: implement platform fiber switch (e.g. SwitchToFiber / swapcontext)
        if (!m_started) {
            m_started = true;
            m_fn();
        }
    }

    /// Yield from inside the fiber back to the scheduler.
    static void yield() {
        // TODO: implement platform fiber yield
    }

    [[nodiscard]] bool isFinished() const { return m_finished; }

private:
    FiberFn m_fn;
    bool    m_started{false};
    bool    m_finished{false};
};

} // namespace shooter
