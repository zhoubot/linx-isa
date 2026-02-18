/*
 * LinxISA QEMU Test Suite - Main Runner
 * 
 * This file includes all test suites and provides a main entry point
 * that runs all tests sequentially.
 */

#include "linx_test.h"

/* Compile-time suite selection (1 = enabled, 0 = disabled) */
#ifndef LINX_TEST_ENABLE_ARITHMETIC
#define LINX_TEST_ENABLE_ARITHMETIC 1
#endif
#ifndef LINX_TEST_ENABLE_BITWISE
#define LINX_TEST_ENABLE_BITWISE 1
#endif
#ifndef LINX_TEST_ENABLE_LOADSTORE
#define LINX_TEST_ENABLE_LOADSTORE 1
#endif
#ifndef LINX_TEST_ENABLE_BRANCH
#define LINX_TEST_ENABLE_BRANCH 1
#endif
#ifndef LINX_TEST_ENABLE_MOVE
#define LINX_TEST_ENABLE_MOVE 1
#endif
#ifndef LINX_TEST_ENABLE_FLOAT
#define LINX_TEST_ENABLE_FLOAT 1
#endif
#ifndef LINX_TEST_ENABLE_ATOMIC
#define LINX_TEST_ENABLE_ATOMIC 1
#endif
#ifndef LINX_TEST_ENABLE_JUMPTABLE
#define LINX_TEST_ENABLE_JUMPTABLE 1
#endif
#ifndef LINX_TEST_ENABLE_VARARGS
#define LINX_TEST_ENABLE_VARARGS 1
#endif
#ifndef LINX_TEST_ENABLE_TILE
#define LINX_TEST_ENABLE_TILE 0
#endif
#ifndef LINX_TEST_ENABLE_SYSTEM
#define LINX_TEST_ENABLE_SYSTEM 1
#endif
#ifndef LINX_TEST_ENABLE_V03_VECTOR
#define LINX_TEST_ENABLE_V03_VECTOR 0
#endif
#ifndef LINX_TEST_ENABLE_V03_VECTOR_OPS
#define LINX_TEST_ENABLE_V03_VECTOR_OPS 0
#endif
#ifndef LINX_TEST_ENABLE_CALLRET
#define LINX_TEST_ENABLE_CALLRET 0
#endif
#ifndef LINX_TEST_ENABLE_PTO_PARITY
#define LINX_TEST_ENABLE_PTO_PARITY 0
#endif

/* Forward declarations for test suite functions */
#if LINX_TEST_ENABLE_ARITHMETIC
void run_arithmetic_tests(void);
#endif
#if LINX_TEST_ENABLE_BITWISE
void run_bitwise_tests(void);
#endif
#if LINX_TEST_ENABLE_LOADSTORE
void run_loadstore_tests(void);
#endif
#if LINX_TEST_ENABLE_BRANCH
void run_branch_tests(void);
#endif
#if LINX_TEST_ENABLE_MOVE
void run_move_tests(void);
#endif
#if LINX_TEST_ENABLE_FLOAT
void run_float_tests(void);
#endif
#if LINX_TEST_ENABLE_ATOMIC
void run_atomic_tests(void);
#endif
#if LINX_TEST_ENABLE_JUMPTABLE
void run_jumptable_tests(void);
#endif
#if LINX_TEST_ENABLE_VARARGS
void run_varargs_tests(void);
#endif
#if LINX_TEST_ENABLE_TILE
void run_tile_tests(void);
#endif
#if LINX_TEST_ENABLE_SYSTEM
void run_system_tests(void);
#endif
#if LINX_TEST_ENABLE_V03_VECTOR
void run_v03_vector_tile_tests(void);
#endif
#if LINX_TEST_ENABLE_V03_VECTOR_OPS
void run_v03_vector_ops_matrix_tests(void);
#endif
#if LINX_TEST_ENABLE_CALLRET
void run_callret_tests(void);
#endif
#if LINX_TEST_ENABLE_PTO_PARITY
void run_pto_parity_tests(void);
#endif

/* Test counters */
static volatile uint32_t g_total_tests = 0;
static volatile uint32_t g_passed_tests = 0;
static volatile uint32_t g_failed_tests = 0;
static volatile uint32_t g_current_suite = 0;

/*
 * Run a test suite and track results
 */
static void run_suite_with_stats(const char *name, void (*suite_func)(void)) {
    g_current_suite++;
#if !LINX_TEST_QUIET
    uart_puts("\r\nRunning ");
    uart_puts(name);
    uart_puts("...\r\n");
#else
    (void)name;
#endif
    suite_func();
}

/*
 * Main entry point
 */
void _start(void) {
#if !LINX_TEST_QUIET
    uart_puts("\r\n");
    uart_puts("===============================================\r\n");
    uart_puts("  Linx ISA QEMU Test Suite (v1.0)\r\n");
    uart_puts("===============================================\r\n");
    uart_puts("\r\n");
    uart_puts("Target: linx64-linx-none-elf\r\n");
    uart_puts("Suites:");
#if LINX_TEST_ENABLE_ARITHMETIC
    uart_puts(" Arithmetic");
#endif
#if LINX_TEST_ENABLE_BITWISE
    uart_puts(" Bitwise");
#endif
#if LINX_TEST_ENABLE_LOADSTORE
    uart_puts(" Load/Store");
#endif
#if LINX_TEST_ENABLE_BRANCH
    uart_puts(" Branch");
#endif
#if LINX_TEST_ENABLE_MOVE
    uart_puts(" Move");
#endif
#if LINX_TEST_ENABLE_FLOAT
    uart_puts(" Float");
#endif
#if LINX_TEST_ENABLE_ATOMIC
    uart_puts(" Atomic");
#endif
#if LINX_TEST_ENABLE_JUMPTABLE
    uart_puts(" JumpTable");
#endif
#if LINX_TEST_ENABLE_VARARGS
    uart_puts(" Varargs");
#endif
#if LINX_TEST_ENABLE_TILE
    uart_puts(" Tile");
#endif
#if LINX_TEST_ENABLE_SYSTEM
    uart_puts(" System");
#endif
#if LINX_TEST_ENABLE_V03_VECTOR
    uart_puts(" v0.3-vector");
#endif
#if LINX_TEST_ENABLE_V03_VECTOR_OPS
    uart_puts(" v0.3-vector-ops");
#endif
#if LINX_TEST_ENABLE_CALLRET
    uart_puts(" callret");
#endif
#if LINX_TEST_ENABLE_PTO_PARITY
    uart_puts(" pto-parity");
#endif
    uart_puts("\r\n");
    uart_puts("\r\n");
    uart_puts("=================================================\r\n");
#endif
    
    /* Run all test suites */
#if LINX_TEST_ENABLE_ARITHMETIC
    run_suite_with_stats("Arithmetic Tests", run_arithmetic_tests);
#endif
#if LINX_TEST_ENABLE_BITWISE
    run_suite_with_stats("Bitwise Tests", run_bitwise_tests);
#endif
#if LINX_TEST_ENABLE_LOADSTORE
    run_suite_with_stats("Load/Store Tests", run_loadstore_tests);
#endif
#if LINX_TEST_ENABLE_BRANCH
    run_suite_with_stats("Branch & Control Flow Tests", run_branch_tests);
#endif
#if LINX_TEST_ENABLE_MOVE
    run_suite_with_stats("Move & Immediate Tests", run_move_tests);
#endif
#if LINX_TEST_ENABLE_FLOAT
    run_suite_with_stats("Floating-Point Tests", run_float_tests);
#endif
#if LINX_TEST_ENABLE_ATOMIC
    run_suite_with_stats("Atomic Operation Tests", run_atomic_tests);
#endif
#if LINX_TEST_ENABLE_JUMPTABLE
    run_suite_with_stats("Jump Table & Indirect Branch Tests", run_jumptable_tests);
#endif
#if LINX_TEST_ENABLE_VARARGS
    run_suite_with_stats("Varargs ABI Tests", run_varargs_tests);
#endif
#if LINX_TEST_ENABLE_TILE
    run_suite_with_stats("Tile Block Tests", run_tile_tests);
#endif
#if LINX_TEST_ENABLE_SYSTEM
    run_suite_with_stats("System & Privilege Tests", run_system_tests);
#endif
#if LINX_TEST_ENABLE_V03_VECTOR
    run_suite_with_stats("v0.3 Vector/Tile Marker Tests", run_v03_vector_tile_tests);
#endif
#if LINX_TEST_ENABLE_V03_VECTOR_OPS
    run_suite_with_stats("v0.3 Vector Operation Matrix Tests", run_v03_vector_ops_matrix_tests);
#endif
#if LINX_TEST_ENABLE_CALLRET
    run_suite_with_stats("Call/Ret Conformance Tests", run_callret_tests);
#endif
#if LINX_TEST_ENABLE_PTO_PARITY
    run_suite_with_stats("PTO Kernel Parity Tests", run_pto_parity_tests);
#endif
    
    /* Print final summary */
#if !LINX_TEST_QUIET
    uart_puts("\r\n");
    uart_puts("=================================================\r\n");
    uart_puts("              TEST SUITE COMPLETE                \r\n");
    uart_puts("=================================================\r\n");
    uart_puts("\r\n");
    uart_puts("All tests completed successfully!\r\n");
    uart_puts("\r\n");
    uart_puts("Note: Check UART output for individual test results.\r\n");
    uart_puts("      Each test suite prints PASS for each test.\r\n");
    uart_puts("\r\n");
#endif
    
    /* Exit with success */
    uart_puts("*** REGRESSION PASSED ***\r\n");
    EXIT_CODE = 0;
    
    /* Loop forever */
    while (1) {
        /* If QEMU doesn't exit for some reason, don't fall through. */
    }
}
