/**
 * @file ht_test.h
 * @brief Minimal dependency-free unit-test harness.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (c) 2024 Michael Berry
 * @license MIT
 *
 * A small xUnit-style harness in portable C11. Each test is a void(void)
 * function run by RUN_TEST; a failed assertion records the failure and
 * longjmps back to the runner so the remaining tests still run. The process
 * exit status is zero only when every test passed.
 *
 * The harness is header-only with internal linkage, so it is included in
 * exactly one translation unit per test executable.
 */

#ifndef HT_TEST_H
#define HT_TEST_H

#include <setjmp.h>
#include <stdio.h>
#include <string.h>

/// Harness state for one test executable.
typedef struct {
    const char *suite;   ///< suite label
    const char *current; ///< running test's name
    int passed;          ///< tests with no failed assertion
    int failed;          ///< tests with at least one failed assertion
    int current_failed;  ///< failed assertions in the running test
    jmp_buf escape;      ///< abort target for a failed assertion
} ht_test_ctx_t;

static ht_test_ctx_t ht_test__ctx;

/// Begin a suite, resetting the counters.
static void ht_test_begin(const char *suite) {
    ht_test__ctx.suite = suite;
    ht_test__ctx.current = NULL;
    ht_test__ctx.passed = 0;
    ht_test__ctx.failed = 0;
    ht_test__ctx.current_failed = 0;
    printf("== %s ==\n", suite);
}

/// Run one test under the failure-abort guard.
static void ht_test__run(const char *name, void (*fn)(void)) {
    ht_test__ctx.current = name;
    ht_test__ctx.current_failed = 0;

    if (setjmp(ht_test__ctx.escape) == 0) {
        fn();
    }

    if (ht_test__ctx.current_failed == 0) {
        ht_test__ctx.passed++;
        printf("  ok   %s\n", name);
    } else {
        ht_test__ctx.failed++;
        printf("  FAIL %s\n", name);
    }
}

/// Print the summary and return a process exit status.
static int ht_test_summary(void) {
    printf("-- %s: %d passed, %d failed --\n", ht_test__ctx.suite,
           ht_test__ctx.passed, ht_test__ctx.failed);
    return ht_test__ctx.failed == 0 ? 0 : 1;
}

/// Register and run a test function by name.
#define RUN_TEST(fn) ht_test__run(#fn, fn)

/// Record a failed assertion at the call site and abort the current test.
#define HT_TEST__FAIL_HEAD()                                                   \
    fprintf(stderr, "  FAIL %s:%d in %s: ", __FILE__, __LINE__,                \
            ht_test__ctx.current)

#define HT_TEST__ABORT()                                                       \
    do {                                                                       \
        ht_test__ctx.current_failed++;                                         \
        longjmp(ht_test__ctx.escape, 1);                                       \
    } while (0)

#define ASSERT_TRUE(cond)                                                      \
    do {                                                                       \
        if (!(cond)) {                                                         \
            HT_TEST__FAIL_HEAD();                                              \
            fprintf(stderr, "expected true: %s\n", #cond);                     \
            HT_TEST__ABORT();                                                  \
        }                                                                      \
    } while (0)

#define ASSERT_FALSE(cond)                                                     \
    do {                                                                       \
        if (cond) {                                                            \
            HT_TEST__FAIL_HEAD();                                              \
            fprintf(stderr, "expected false: %s\n", #cond);                    \
            HT_TEST__ABORT();                                                  \
        }                                                                      \
    } while (0)

#define ASSERT_EQ_INT(a, b)                                                    \
    do {                                                                       \
        long long _a = (long long)(a), _b = (long long)(b);                    \
        if (_a != _b) {                                                        \
            HT_TEST__FAIL_HEAD();                                              \
            fprintf(stderr, "%s == %s (%lld != %lld)\n", #a, #b, _a, _b);      \
            HT_TEST__ABORT();                                                  \
        }                                                                      \
    } while (0)

#define ASSERT_EQ_UINT(a, b)                                                   \
    do {                                                                       \
        unsigned long long _a = (unsigned long long)(a);                       \
        unsigned long long _b = (unsigned long long)(b);                       \
        if (_a != _b) {                                                        \
            HT_TEST__FAIL_HEAD();                                              \
            fprintf(stderr, "%s == %s (%llu != %llu)\n", #a, #b, _a, _b);      \
            HT_TEST__ABORT();                                                  \
        }                                                                      \
    } while (0)

#define ASSERT_EQ_PTR(a, b)                                                    \
    do {                                                                       \
        const void *_a = (const void *)(a), *_b = (const void *)(b);           \
        if (_a != _b) {                                                        \
            HT_TEST__FAIL_HEAD();                                              \
            fprintf(stderr, "%s == %s (%p != %p)\n", #a, #b, _a, _b);          \
            HT_TEST__ABORT();                                                  \
        }                                                                      \
    } while (0)

#define ASSERT_NULL(p)                                                         \
    do {                                                                       \
        const void *_p = (const void *)(p);                                    \
        if (_p != NULL) {                                                      \
            HT_TEST__FAIL_HEAD();                                              \
            fprintf(stderr, "expected NULL: %s (%p)\n", #p, _p);               \
            HT_TEST__ABORT();                                                  \
        }                                                                      \
    } while (0)

#define ASSERT_NOT_NULL(p)                                                     \
    do {                                                                       \
        if ((const void *)(p) == NULL) {                                       \
            HT_TEST__FAIL_HEAD();                                              \
            fprintf(stderr, "expected non-NULL: %s\n", #p);                    \
            HT_TEST__ABORT();                                                  \
        }                                                                      \
    } while (0)

#define ASSERT_STR_EQ(a, b)                                                    \
    do {                                                                       \
        const char *_a = (a), *_b = (b);                                       \
        if (_a == NULL || _b == NULL || strcmp(_a, _b) != 0) {                 \
            HT_TEST__FAIL_HEAD();                                              \
            fprintf(stderr, "%s == %s (\"%s\" != \"%s\")\n", #a, #b,           \
                    _a ? _a : "(null)", _b ? _b : "(null)");                   \
            HT_TEST__ABORT();                                                  \
        }                                                                      \
    } while (0)

#define ASSERT_MEM_EQ(a, b, n)                                                 \
    do {                                                                       \
        const void *_a = (a), *_b = (b);                                       \
        size_t _n = (size_t)(n);                                               \
        if (memcmp(_a, _b, _n) != 0) {                                         \
            HT_TEST__FAIL_HEAD();                                              \
            fprintf(stderr, "%zu bytes differ: %s vs %s\n", _n, #a, #b);       \
            HT_TEST__ABORT();                                                  \
        }                                                                      \
    } while (0)

#define ASSERT_EQ_DBL(a, b, eps)                                               \
    do {                                                                       \
        double _a = (a), _b = (b), _e = (eps);                                 \
        double _d = _a - _b;                                                   \
        if (_d < 0) {                                                          \
            _d = -_d;                                                          \
        }                                                                      \
        if (_d > _e) {                                                         \
            HT_TEST__FAIL_HEAD();                                              \
            fprintf(stderr, "%s == %s (%g vs %g, eps %g)\n", #a, #b, _a, _b,   \
                    _e);                                                       \
            HT_TEST__ABORT();                                                  \
        }                                                                      \
    } while (0)

/// Fail the current test unconditionally with a fixed message.
#define FAIL_MSG(msg)                                                          \
    do {                                                                       \
        HT_TEST__FAIL_HEAD();                                                  \
        fprintf(stderr, "%s\n", msg);                                          \
        HT_TEST__ABORT();                                                      \
    } while (0)

#endif /* HT_TEST_H */
