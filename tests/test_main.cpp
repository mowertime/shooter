/**
 * ShooterEngine unit tests – single-binary test runner.
 *
 * Tests are organised by subsystem and run sequentially.
 * Each ASSERT increments a global pass/fail counter.
 */

#include <cstdio>
#include <cmath>
#include <cstring>

// ── Minimal test framework ────────────────────────────────────────────────────

int g_pass = 0;
int g_fail = 0;
const char* g_suiteName = "Global"; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

#define SUITE(name) g_suiteName = (name)

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

#define ASSERT_NEAR(a, b, tol) \
    ASSERT(std::fabs((float)(a) - (float)(b)) < (float)(tol))

// ── Forward declarations of test suites ──────────────────────────────────────
void test_ecs();
void test_physics();
void test_ai();
void test_world();
void test_audio();
void test_ballistics();
void test_threading();

int main() {
    std::printf("=== ShooterEngine Unit Tests ===\n");

    test_ecs();
    test_physics();
    test_ai();
    test_world();
    test_audio();
    test_ballistics();
    test_threading();

    std::printf("\n=== Results: %d passed, %d failed ===\n", g_pass, g_fail);
    return g_fail > 0 ? 1 : 0;
}
