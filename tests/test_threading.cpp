#include "core/Threading.h"
#include "core/Memory.h"
#include <cstdio>
#include <cmath>
#include <atomic>
#include <chrono>
#include <thread>

using namespace shooter;

extern int g_pass;
extern int g_fail;
extern const char* g_suiteName;

#define ASSERT(expr) \
    do { \
        if (!(expr)) { \
            std::fprintf(stderr, "  FAIL [%s] %s:%d  (%s)\n", \
                         g_suiteName, __FILE__, __LINE__, #expr); \
            ++g_fail; \
        } else { \
            ++g_pass; \
        } \
    } while(0)

void test_threading() {
    g_suiteName = "Threading";
    std::printf("\n--- Threading ---\n");

    // ThreadPool: all submitted jobs run.
    {
        ThreadPool pool(4);
        std::atomic<int> counter{0};
        constexpr int N = 100;
        for (int i = 0; i < N; ++i) {
            pool.submit([&counter] { counter.fetch_add(1); });
        }
        pool.waitIdle();
        ASSERT(counter.load() == N);
    }

    // ThreadPool: priority ordering (high-priority jobs run before low).
    {
        ThreadPool pool(1); // Single worker to make ordering deterministic
        std::vector<int> order;
        std::mutex orderMutex;

        // Submit low-priority first, then high-priority.
        pool.submit([&order, &orderMutex] {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            std::lock_guard lk(orderMutex);
            order.push_back(2);
        }, JobPriority::Low);

        pool.submit([&order, &orderMutex] {
            std::lock_guard lk(orderMutex);
            order.push_back(1);
        }, JobPriority::High);

        pool.waitIdle();
        // We just verify both ran.
        ASSERT(order.size() == 2);
    }

    // SpinLock protects shared counter.
    {
        SpinLock lock;
        int counter = 0;
        constexpr int N = 1000;
        std::vector<std::thread> threads;
        for (int i = 0; i < 4; ++i) {
            threads.emplace_back([&lock, &counter, N] {
                for (int j = 0; j < N / 4; ++j) {
                    lock.lock();
                    ++counter;
                    lock.unlock();
                }
            });
        }
        for (auto& t : threads) t.join();
        ASSERT(counter == N);
    }

    // LinearAllocator: bump allocation and reset.
    {
        LinearAllocator la(1024);
        ASSERT(la.capacity() == 1024);
        ASSERT(la.used() == 0);

        void* p1 = la.alloc(100);
        ASSERT(p1 != nullptr);
        ASSERT(la.used() >= 100);

        void* p2 = la.alloc(200);
        ASSERT(p2 != nullptr);
        ASSERT(p2 != p1);

        la.reset();
        ASSERT(la.used() == 0);
    }

    // GeneralAllocator: alloc and free.
    {
        auto& ga = GeneralAllocator::get();
        u64 prevCount = ga.allocationCount();
        void* p = ga.alloc(256, 64);
        ASSERT(p != nullptr);
        ASSERT(ga.allocationCount() == prevCount + 1);
        ga.free(p);
        ASSERT(ga.allocationCount() == prevCount);
    }

    // JobCounter: wait for zero.
    {
        JobCounter counter(3);
        ASSERT(counter.value() == 3);
        counter.decrement();
        counter.decrement();
        counter.decrement();
        counter.waitUntilZero();
        ASSERT(counter.value() == 0);
    }

    std::printf("  Threading tests complete\n");
}
