/*
 * LinxISA QEMU Test Framework
 * 
 * This header provides utilities for writing unit tests that run on QEMU.
 *
 * Conventions:
 * - UART (0x10000000) is used for human-readable output.
 * - EXIT register (0x10000004) is used to request QEMU shutdown with an exit
 *   status code (0 = PASS, non-zero = FAIL). Do not write EXIT_CODE for each
 *   passing test; only write it for final termination (or on failure).
 * - On failure, a small debug record is written to TEST_RESULT_LOC
 *   (0x00008000) for automated triage.
 */

#ifndef LINX_TEST_H
#define LINX_TEST_H

#include <stdint.h>

/* UART printing controls.
 *
 * Printing a line per test is very slow under the TCG interpreter and can
 * dominate runtime. Default to verbose output unless the harness sets
 * LINX_TEST_QUIET=1.
 */
#ifndef LINX_TEST_QUIET
#define LINX_TEST_QUIET 0
#endif

/* Test result codes */
#define TEST_PASS     0
#define TEST_FAIL     1
#define TEST_ABORT    2

/* UART and Exit Register addresses */
#define UART_BASE     0x10000000
#define EXIT_REG      0x10000004
#define UART_DR       (*(volatile uint32_t *)(UART_BASE + 0x00))
#define EXIT_CODE     (*(volatile uint32_t *)(EXIT_REG))

/* Magic number for test results: "LXTEST" */
#define TEST_MAGIC    0x4C58455453UL

/* Test result structure (written to exit_reg) */
typedef struct {
    uint64_t magic;      /* TEST_MAGIC */
    uint32_t test_id;    /* Test identifier */
    uint32_t result;     /* TEST_PASS or TEST_FAIL */
    uint64_t expected;   /* Expected value */
    uint64_t actual;     /* Actual computed value */
    uint64_t pc;         /* Program counter at completion */
} __attribute__((packed)) test_result_t;

/* Global test result storage (at fixed memory location in RAM) */
#define TEST_RESULT_LOC  0x00008000
static volatile test_result_t *g_test_result = (volatile test_result_t *)TEST_RESULT_LOC;

/*
 * Output a character to UART
 */
static inline void uart_putc(char c) {
    UART_DR = (uint32_t)(unsigned char)c;
}

/*
 * Output a string to UART
 */
static inline void uart_puts(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

/*
 * Output a hex digit
 */
static inline void uart_puthex_digit(uint8_t d) {
    uart_putc("0123456789ABCDEF"[d & 0xFu]);
}

/*
 * Output a 32-bit hex number
 */
static inline void uart_puthex32(uint32_t v) {
    for (int i = 28; i >= 0; i -= 4) {
        uart_puthex_digit((v >> i) & 0xF);
    }
}

/*
 * Output a 64-bit hex number
 */
static inline void uart_puthex64(uint64_t v) {
    uart_puthex32((uint32_t)(v >> 32));
    uart_puthex32((uint32_t)(v & 0xFFFFFFFF));
}

/*
 * Begin a test suite
 */
static inline void test_suite_begin(uint32_t suite_id) {
#if !LINX_TEST_QUIET
    uart_puts("\r\n=== Test Suite ");
    uart_puts("0x");
    uart_puthex32(suite_id);
    uart_puts(" ===\r\n");
#else
    (void)suite_id;
#endif
}

/*
 * Report test start
 */
static inline void test_start(uint32_t test_id) {
#if !LINX_TEST_QUIET
    uart_puts("  Test 0x");
    uart_puthex32(test_id);
    uart_puts(": ");
#else
    (void)test_id;
#endif
}

/*
 * Report test pass
 */
static inline void test_pass(void) {
#if !LINX_TEST_QUIET
    uart_puts("PASS\r\n");
#endif
}

/*
 * Report test fail with details
 */
static inline void test_fail(uint32_t test_id, uint64_t expected, uint64_t actual) {
    uart_puts("FAIL\r\n");
    uart_puts("    Test ID:  0x");
    uart_puthex32(test_id);
    uart_puts("\r\n");
    uart_puts("    Expected: 0x");
    uart_puthex64(expected);
    uart_puts("\r\n");
    uart_puts("    Actual:   0x");
    uart_puthex64(actual);
    uart_puts("\r\n");
    
    /* Store result for automated checking */
    g_test_result->magic = TEST_MAGIC;
    g_test_result->test_id = test_id;
    g_test_result->result = TEST_FAIL;
    g_test_result->expected = expected;
    g_test_result->actual = actual;
    
    EXIT_CODE = TEST_FAIL;
    while(1) {} /* Hang on failure */
}

/*
 * Assert that condition is true
 */
#define TEST_ASSERT(cond, test_id, expected, actual) do { \
    if (!(cond)) { \
        test_fail((test_id), (expected), (actual)); \
    } \
} while(0)

/*
 * Assert two values are equal
 */
#define TEST_EQ(actual, expected, test_id) do { \
    uint64_t _a = (uint64_t)(actual); \
    uint64_t _e = (uint64_t)(expected); \
    if (_a != _e) { \
        test_fail((test_id), _e, _a); \
    } \
} while(0)

/*
 * Assert two 32-bit values are equal
 */
#define TEST_EQ32(actual, expected, test_id) do { \
    uint32_t _a = (uint32_t)(actual); \
    uint32_t _e = (uint32_t)(expected); \
    if (_a != _e) { \
        test_fail((test_id), _e, _a); \
    } \
} while(0)

/*
 * Assert two 64-bit values are equal
 */
#define TEST_EQ64(actual, expected, test_id) do { \
    uint64_t _a = (uint64_t)(actual); \
    uint64_t _e = (uint64_t)(expected); \
    if (_a != _e) { \
        test_fail((test_id), _e, _a); \
    } \
} while(0)

/*
 * Assert two floating point values are approximately equal
 */
#define TEST_EQF(actual, expected, test_id, tolerance) do { \
    double _a = (double)(actual); \
    double _e = (double)(expected); \
    union { double d; uint64_t u; } _abits = { .d = _a }; \
    union { double d; uint64_t u; } _ebits = { .d = _e }; \
    /* Fast-path exact matches (also treats +0/-0 as equal). */ \
    if (((_abits.u ^ _ebits.u) == 0) || (((_abits.u | _ebits.u) & 0x7FFFFFFFFFFFFFFFULL) == 0)) { \
        break; \
    } \
    double _diff = _a - _e; \
    if (_diff < 0) _diff = -_diff; \
    if (_diff > (tolerance)) { \
        uart_puts("FAIL (float tolerance exceeded)\r\n"); \
        test_fail((test_id), _ebits.u, _abits.u); \
    } \
} while(0)

/*
 * Run a test and report result
 */
#define RUN_TEST(name, id) do { \
    test_start(id); \
    name(); \
    test_pass(); \
} while(0)

/*
 * End of test suite - print summary
 */
static inline void test_suite_end(uint32_t total, uint32_t passed) {
#if !LINX_TEST_QUIET
    uart_puts("\r\nSuite Results: ");
    uart_puts("0x");
    uart_puthex32(passed);
    uart_puts("/0x");
    uart_puthex32(total);
    uart_puts(" passed\r\n");
    uart_puts("===================\r\n");
#else
    (void)total;
    (void)passed;
#endif
}

/*
 * Exit test suite with final result
 */
static inline void test_suite_exit(uint32_t passed, uint32_t total) {
    if (passed == total) {
        uart_puts("\r\n*** ALL TESTS PASSED ***\r\n");
        EXIT_CODE = 0;
    } else {
        uart_puts("\r\n*** SOME TESTS FAILED ***\r\n");
        EXIT_CODE = 1;
    }
    while(1) {}
}

/*
 * Delay loop (for QEMU synchronization)
 */
static inline void delay(uint32_t cycles) {
    for (volatile uint32_t i = 0; i < cycles; i++) {
        __asm__ volatile ("");
    }
}

#endif /* LINX_TEST_H */
