#pragma once

// Minimal dependency-free test harness: TEST registers a function, CHECK
// records failures without aborting the test, main() runs everything.

#include <cmath>
#include <cstdio>
#include <functional>
#include <vector>

namespace check {

struct Case {
    const char* name;
    std::function<void()> body;
};

inline std::vector<Case>& registry()
{
    static std::vector<Case> cases;
    return cases;
}

inline int& failures()
{
    static int count = 0;
    return count;
}

struct Registrar {
    Registrar(const char* name, std::function<void()> body)
    {
        registry().push_back({name, std::move(body)});
    }
};

inline int runAll()
{
    for (const Case& c : registry()) {
        const int before = failures();
        c.body();
        std::printf("%s %s\n", failures() == before ? "PASS" : "FAIL", c.name);
    }
    std::printf("%zu tests, %d failed checks\n", registry().size(), failures());
    return failures() == 0 ? 0 : 1;
}

} // namespace check

#define CHECK_CONCAT_(a, b) a##b
#define CHECK_CONCAT(a, b) CHECK_CONCAT_(a, b)

#define TEST(name)                                                                   \
    static void CHECK_CONCAT(test_, __LINE__)();                                     \
    static check::Registrar CHECK_CONCAT(registrar_, __LINE__)(name, CHECK_CONCAT(test_, __LINE__)); \
    static void CHECK_CONCAT(test_, __LINE__)()

#define CHECK(cond)                                                                  \
    do {                                                                             \
        if (!(cond)) {                                                               \
            ++check::failures();                                                     \
            std::printf("  %s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #cond);   \
        }                                                                            \
    } while (0)

#define CHECK_NEAR(actual, expected, tolerance)                                      \
    do {                                                                             \
        const double a_ = (actual), e_ = (expected), t_ = (tolerance);              \
        if (!(std::fabs(a_ - e_) <= t_)) {                                           \
            ++check::failures();                                                     \
            std::printf("  %s:%d: %s = %g, expected %g +/- %g\n", __FILE__, __LINE__, \
                        #actual, a_, e_, t_);                                        \
        }                                                                            \
    } while (0)
