/*
 * LinxISA System/Privilege Unit Tests (QEMU)
 *
 * This suite validates:
 * - Base SSR access (SSRGET/SSRSET/SSRSWAP) including symbolic SSR names
 * - HL.SSRGET/HL.SSRSET for extended SSR IDs (e.g. 0x1Fxx)
 * - ACRE/ACRC context switches (SERVICE_REQUEST + ACR_ENTER)
 * - A basic timer interrupt routed to ACR1
 *
 * Notes:
 * - Handlers are written in assembly to avoid stack/prologue side effects,
 *   because QEMU vectors to EVBASE by setting PC (not by a normal call/return).
 * - Continuation PCs are passed via ETEMP/ETEMP0 (ACR1) and scratch SSR 0x0035
 *   using addresses of `noreturn` C stage functions (function-entry markers
 *   are valid block start targets in the Linx Block ISA bring-up rules).
 */

#include "linx_test.h"
#include <stdint.h>

/* Scratch SSRs (non-privileged) used for test communication. */
enum {
    SSR_SCRATCH0 = 0x0030, /* general R/W */
    SSR_SYSCALL_SEEN = 0x0031,
    SSR_IRQ_SEEN = 0x0032,
    SSR_CONT_EXIT = 0x0035,
    SSR_LAST_TRAPNO = 0x0036,
    SSR_LAST_TRAPARG0 = 0x0037,
    SSR_LAST_EBARG_TPC = 0x0038,
    SSR_LAST_ECSTATE = 0x0039,
    SSR_ACR0_TRAPNO = 0x003A,
    SSR_ACR0_TRAPARG0 = 0x003B,
    SSR_ACR0_ECSTATE = 0x003C,
    SSR_IRQ_SEEN_BEFORE_ENABLE = 0x003D,
    SSR_BP_RESUME_SEEN = 0x003E,
};

/* Managing-ACR SSR IDs (ACR0 fits in 12-bit; ACR1 requires HL). */
enum {
    SSR_ECSTATE_ACR0 = 0x0F00,
    SSR_EVBASE_ACR0 = 0x0F01,
    SSR_EBARG_BPC_CUR_ACR0 = 0x0F41,
    SSR_EBARG_TPC_ACR0 = 0x0F43,

    SSR_EVBASE_ACR1 = 0x1F01,
    SSR_TRAPNO_ACR1 = 0x1F02,
    SSR_TRAPARG0_ACR1 = 0x1F03,
    SSR_ETEMP_ACR1 = 0x1F05,
    SSR_ETEMP0_ACR1 = 0x1F06,
    SSR_EBARG_BPC_CUR_ACR1 = 0x1F41,
    SSR_EBARG_TPC_ACR1 = 0x1F43,
    SSR_TIMER_TIMECMP_ACR1 = 0x1F21,

    /* v0.2 debug SSRs (bring-up subset). */
    SSR_DBCR0_ACR2 = 0x2F90,
    SSR_DBVR0_ACR2 = 0x2F91,
    SSR_DWCR0_ACR2 = 0x2FB0,
    SSR_DWVR0_ACR2 = 0x2FB1,
};

/* Test IDs */
enum {
    TESTID_SSR_BASIC = 0x1100,
    TESTID_SSR_HL = 0x1101,
    TESTID_PRIV_FLOW = 0x1102,
    TESTID_ACRC_ADJ = 0x1103,
    TESTID_DBG_BP = 0x1104,
    TESTID_DBG_WP = 0x1105,
    TESTID_ACR_ROUTE_A2_MAC = 0x1106,
    TESTID_ACR_ROUTE_A1_SEC = 0x1107,
    TESTID_IRQ_GATE_ACR1 = 0x1108,
    TESTID_ACR1_BAD_REQ = 0x1109,
    TESTID_IRQ_PREEMPT_A2 = 0x110A,
    TESTID_IRQ_META_A2 = 0x110B,
    TESTID_ACRE_BAD_TARGET = 0x110C,
    TESTID_ACR0_BAD_REQ = 0x110D,
    TESTID_DBG_BP_RESUME = 0x110E,
};

__attribute__((noreturn)) static void linx_priv_user_code(void);
__attribute__((noreturn)) static void linx_priv_after_syscall(void);
__attribute__((noreturn)) static void linx_priv_after_irq(void);
__attribute__((noreturn)) static void linx_priv_after_exit(void);
__attribute__((noreturn)) static void linx_dbg_wp_user(void);
__attribute__((noreturn)) static void linx_after_bad_acrc_exit(void);
__attribute__((noreturn)) static void linx_after_dbg_bp_exit(void);
__attribute__((noreturn)) static void linx_after_dbg_bp_resume_exit(void);
__attribute__((noreturn)) static void linx_after_dbg_wp_exit(void);
__attribute__((noreturn)) static void linx_after_acr2_mac_exit(void);
__attribute__((noreturn)) static void linx_after_acr1_sec_exit(void);
__attribute__((noreturn)) static void linx_acr1_irq_gate_user(void);
__attribute__((noreturn)) static void linx_acr1_irq_gate_after(void);
__attribute__((noreturn)) static void linx_after_irq_gate_exit(void);
__attribute__((noreturn)) static void linx_after_acr1_bad_req_trap(void);
__attribute__((noreturn)) static void linx_after_acr1_bad_req_exit(void);
__attribute__((noreturn)) static void linx_acr2_irq_preempt_user(void);
__attribute__((noreturn)) static void linx_acr2_irq_preempt_after(void);
__attribute__((noreturn)) static void linx_after_acr2_irq_preempt_exit(void);
__attribute__((noreturn)) static void linx_acr2_irq_meta_user(void);
__attribute__((noreturn)) static void linx_acr2_irq_meta_after(void);
__attribute__((noreturn)) static void linx_after_irq_meta_exit(void);
__attribute__((noreturn)) static void linx_after_acr1_bad_target_trap(void);
__attribute__((noreturn)) static void linx_after_acr1_bad_target_exit(void);
__attribute__((noreturn)) static void linx_after_acr0_bad_req_exit(void);
__attribute__((noreturn)) static void linx_system_done(void);

static volatile uint64_t WATCH_TARGET;

/* v0.2 TRAPNO encoding helpers (E/ARGV/CAUSE/TRAPNUM). */
static inline uint64_t trapno_is_async(uint64_t trapno) { return (trapno >> 63) & 1ull; }
static inline uint64_t trapno_has_argv(uint64_t trapno) { return (trapno >> 62) & 1ull; }
static inline uint64_t trapno_cause(uint64_t trapno) { return (trapno >> 24) & 0xFFFFFFull; }
static inline uint64_t trapno_trapnum(uint64_t trapno) { return trapno & 0x3Full; }
#define CSTATE_I_BIT (1ull << 4)
#define CSTATE_ACR_MASK 0xFull

static inline uint64_t ssrget_uimm(uint32_t ssrid)
{
    uint64_t out;
    __asm__ volatile("ssrget %1, ->%0" : "=r"(out) : "i"(ssrid) : "memory");
    return out;
}

static inline void ssrset_uimm(uint32_t ssrid, uint64_t value)
{
    __asm__ volatile("ssrset %0, %1" : : "r"(value), "i"(ssrid) : "memory");
}

static inline uint64_t ssrswap_uimm(uint32_t ssrid, uint64_t value)
{
    uint64_t old;
    __asm__ volatile("ssrswap %1, %2, ->%0" : "=r"(old) : "r"(value), "i"(ssrid) : "memory");
    return old;
}

static inline uint64_t ssrget_time_symbol(void)
{
    uint64_t out;
    __asm__ volatile("ssrget TIME, ->%0" : "=r"(out) : : "memory");
    return out;
}

static inline uint64_t ssrget_cycle_symbol(void)
{
    uint64_t out;
    /* Ensures LLVM's assembler maps CYCLE to 0x0C00 (per isa-draft). */
    __asm__ volatile("ssrget CYCLE, ->%0" : "=r"(out) : : "memory");
    return out;
}

static inline uint64_t ssrget_cstate_symbol(void)
{
    uint64_t out;
    __asm__ volatile("ssrget CSTATE, ->%0" : "=r"(out) : : "memory");
    return out;
}

static inline void ssrset_cstate_symbol(uint64_t value)
{
    __asm__ volatile("ssrset %0, CSTATE" : : "r"(value) : "memory");
}

static inline uint64_t hl_ssrget_uimm24(uint32_t ssrid)
{
    uint64_t out;
    __asm__ volatile("hl.ssrget %1, ->%0" : "=r"(out) : "i"(ssrid) : "memory");
    return out;
}

static inline void hl_ssrset_uimm24(uint32_t ssrid, uint64_t value)
{
    __asm__ volatile("hl.ssrset %0, %1" : : "r"(value), "i"(ssrid) : "memory");
}

extern void linx_acr1_syscall_handler(void);
extern void linx_acr1_timer_handler(void);
extern void linx_acr0_exit_handler(void);
extern void linx_acr1_record_trap_handler(void);
extern void linx_acr1_bp_resume_handler(void);
extern void linx_bad_acrc_user(void);
extern void linx_trap_resume_to_exit(void);
extern void linx_dbg_bp_user(void);
extern void linx_dbg_bp_resume_user(void);
extern void linx_acr2_mac_user(void);
extern void linx_acr1_sec_user(void);
extern void linx_acr1_bad_req_user(void);
extern void linx_acr1_bad_target_user(void);

/* ACR1 syscall handler:
 * - mark seen (SSR_SYSCALL_SEEN=1)
 * - read continuation PC from ETEMP0_ACR1
 * - write EBARG_TPC_ACR1 to continuation and return via ACRE
 */
__asm__(
    ".globl linx_acr1_syscall_handler\n"
    "linx_acr1_syscall_handler:\n"
    "  C.BSTART\n"
    "  hl.ssrget 0x1f06, ->a0\n" /* ETEMP0_ACR1: continuation PC */
    "  addi zero, 1, ->a1\n"
    "  ssrset a1, 0x0031\n"     /* syscall seen */
    "  hl.ssrset a0, 0x1f41\n"  /* EBARG_BPC_CUR_ACR1 = cont */
    "  hl.ssrset a0, 0x1f43\n"  /* EBARG_TPC_ACR1 = cont */
    "  acre 0\n"
);

/* ACR1 timer interrupt handler:
 * - mark seen (SSR_IRQ_SEEN=1)
 * - cancel TIMECMP (disable re-fire)
 * - read continuation PC from ETEMP_ACR1
 * - write EBARG_BPC_CUR_ACR1 and return via ACRE
 */
__asm__(
    ".globl linx_acr1_timer_handler\n"
    "linx_acr1_timer_handler:\n"
    "  C.BSTART\n"
    "  addi zero, 1, ->a1\n"
    "  ssrset a1, 0x0032\n"     /* irq seen */
    "  addi zero, 0, ->a1\n"
    "  hl.ssrset a1, 0x1f21\n"  /* TIMECMP=0 (cancel) */
    "  hl.ssrget 0x1f05, ->a0\n" /* ETEMP_ACR1: continuation PC */
    "  hl.ssrset a0, 0x1f41\n"  /* EBARG_BPC_CUR_ACR1 = cont */
    "  hl.ssrset a0, 0x1f43\n"  /* EBARG_TPC_ACR1 = cont */
    "  acre 0\n"
);

/* ACR0 exit handler (service request from ACR2):
 * - set ECSTATE_ACR0.ACR = 0 (return to ACR0)
 * - snapshot ACR0 trap metadata (TRAPNO/TRAPARG0/ECSTATE)
 * - read continuation PC from SSR_CONT_EXIT
 * - write EBARG_BPC_CUR_ACR0 and return via ACRE
 */
__asm__(
    ".globl linx_acr0_exit_handler\n"
    "linx_acr0_exit_handler:\n"
    "  C.BSTART\n"
    "  ssrget 0x0f02, ->a2\n"
    "  ssrset a2, 0x003a\n"
    "  ssrget 0x0f03, ->a2\n"
    "  ssrset a2, 0x003b\n"
    "  ssrget 0x0f00, ->a2\n"
    "  ssrset a2, 0x003c\n"
    "  addi zero, 0, ->a1\n"
    "  ssrset a1, 0x0f00\n"     /* target ACR0 */
    "  ssrget 0x0035, ->a0\n"   /* continuation PC */
    "  ssrset a0, 0x0f41\n"     /* EBARG_BPC_CUR_ACR0 = cont */
    "  ssrset a0, 0x0f43\n"     /* EBARG_TPC_ACR0 = cont */
    "  acre 0\n"
);

/* ACR1 generic trap recorder (v0.2 TRAPNO + EBARG.TPC):
 * - snapshot trapno/traparg0/ebarg_tpc into scratch SSRs
 * - return to a fixed ACR2 resume block from ETEMP0_ACR1 via EBARG
 */
__asm__(
    ".globl linx_acr1_record_trap_handler\n"
    "linx_acr1_record_trap_handler:\n"
    "  C.BSTART\n"
    "  hl.ssrget 0x1f02, ->a0\n"
    "  hl.ssrget 0x1f03, ->a1\n"
    "  hl.ssrget 0x1f43, ->a2\n"
    "  hl.ssrget 0x1f00, ->a4\n"
    "  ssrset a0, 0x0036\n"
    "  ssrset a1, 0x0037\n"
    "  ssrset a2, 0x0038\n"
    "  ssrset a4, 0x0039\n"
    "  addi zero, 0, ->a5\n"
    "  hl.ssrset a5, 0x1f21\n"  /* TIMECMP=0 (cancel any timer IRQ re-fire) */
    "  hl.ssrget 0x1f06, ->a3\n" /* ETEMP0_ACR1: trap continuation */
    "  hl.ssrset a3, 0x1f41\n"
    "  hl.ssrset a3, 0x1f43\n"
    "  acre 0\n"
);

/* ACR1 breakpoint resume handler:
 * - snapshot trap metadata into scratch SSRs
 * - resume trapped ACR2 body using captured EBARG_TPC
 */
__asm__(
    ".globl linx_acr1_bp_resume_handler\n"
    "linx_acr1_bp_resume_handler:\n"
    "  C.BSTART\n"
    "  hl.ssrget 0x1f02, ->a0\n" /* TRAPNO_ACR1 */
    "  hl.ssrget 0x1f03, ->a1\n" /* TRAPARG0_ACR1 */
    "  hl.ssrget 0x1f43, ->a2\n" /* EBARG_TPC_ACR1 (captured next PC) */
    "  hl.ssrget 0x1f00, ->a4\n" /* ECSTATE_ACR1 */
    "  ssrset a0, 0x0036\n"
    "  ssrset a1, 0x0037\n"
    "  ssrset a2, 0x0038\n"
    "  ssrset a4, 0x0039\n"
    "  addi zero, 0, ->a5\n"
    "  hl.ssrset a5, 0x1f21\n"  /* TIMECMP=0 (cancel any timer IRQ re-fire) */
    "  hl.ssrset a2, 0x1f41\n"  /* resume block start at captured continuation */
    "  hl.ssrset a2, 0x1f43\n"
    "  acre 0\n"
);

/* ACR2 resume block after a recorded trap: exit back to ACR0 (SCT_MAC). */
__asm__(
    ".globl linx_trap_resume_to_exit\n"
    "linx_trap_resume_to_exit:\n"
    "  C.BSTART\n"
    "  acrc 0\n"
    "  C.BSTOP\n"
);

/* ACR2 negative test: ACRC must be followed immediately by C.BSTOP. */
__asm__(
    ".globl linx_bad_acrc_user\n"
    "linx_bad_acrc_user:\n"
    "  C.BSTART\n"
    "  acrc 1\n"
    "  addi zero, 0, ->a0\n"
    "  C.BSTOP\n"
);

/* ACR2 breakpoint trigger: hit a 32-bit ADDI at a fixed offset (pc+2). */
__asm__(
    ".globl linx_dbg_bp_user\n"
    "linx_dbg_bp_user:\n"
    "  C.BSTART\n"
    "  addi zero, 0, ->a0\n"
    "  C.BSTOP\n"
);

/* ACR2 breakpoint-resume stage:
 * - first ADDI is trapped by BP0
 * - on resume, executes marker write then exits to ACR0
 */
__asm__(
    ".globl linx_dbg_bp_resume_user\n"
    "linx_dbg_bp_resume_user:\n"
    "  C.BSTART\n"
    "  addi zero, 0, ->a0\n"   /* bp target @ +2 */
    "  addi zero, 1, ->a1\n"
    "  ssrset a1, 0x003e\n"    /* SSR_BP_RESUME_SEEN = 1 */
    "  acrc 0\n"
    "  C.BSTOP\n"
);

/* ACR2 user stage: trigger SCT_MAC routing to ACR0. */
__asm__(
    ".globl linx_acr2_mac_user\n"
    "linx_acr2_mac_user:\n"
    "  C.BSTART\n"
    "  acrc 0\n"
    "  C.BSTOP\n"
);

/* ACR1 user stage: trigger SCT_SEC routing to ACR0. */
__asm__(
    ".globl linx_acr1_sec_user\n"
    "linx_acr1_sec_user:\n"
    "  C.BSTART\n"
    "  acrc 2\n"
    "  C.BSTOP\n"
);

/* ACR1 negative test: SCT_SYS is illegal in ACR1 (only SCT_MAC/SCT_SEC). */
__asm__(
    ".globl linx_acr1_bad_req_user\n"
    "linx_acr1_bad_req_user:\n"
    "  C.BSTART\n"
    "  acrc 1\n"
    "  C.BSTOP\n"
);

/* ACR1 negative test: ACRE targeting more-privileged ACR0 must trap. */
__asm__(
    ".globl linx_acr1_bad_target_user\n"
    "linx_acr1_bad_target_user:\n"
    "  C.BSTART\n"
    "  addi zero, 0, ->a0\n"
    "  hl.ssrset a0, 0x1f00\n"  /* ECSTATE_ACR1 target = ACR0 (invalid from ACR1) */
    "  hl.ssrset a0, 0x1f41\n"
    "  acre 1\n"
    "  C.BSTOP\n"
);

__attribute__((noreturn)) static void linx_priv_user_code(void)
{
    /* ACR2: request a syscall (SCT_SYS) which routes to ACR1. */
    __asm__ volatile("acrc 1\n  c.bstop\n" : : : "memory");
    __builtin_unreachable();
}

__attribute__((noreturn)) static void linx_dbg_wp_user(void)
{
    /* ACR2: perform the watched store, then request exit back to ACR0. */
    WATCH_TARGET = 0x1122334455667788ull;
    __asm__ volatile("acrc 0\n  c.bstop\n" : : : "memory");
    __builtin_unreachable();
}

__attribute__((noreturn)) static void linx_priv_after_syscall(void)
{
    const uint64_t cstate = ssrget_cstate_symbol();

    /* Verify that the syscall handler ran. */
    TEST_EQ64(ssrget_uimm(SSR_SYSCALL_SEEN), 1, TESTID_PRIV_FLOW + 1);
    /* ACR2 user stage should run with interrupts masked in this bring-up flow. */
    TEST_EQ64(cstate & CSTATE_ACR_MASK, 2, TESTID_PRIV_FLOW + 4);
    TEST_EQ64((cstate & CSTATE_I_BIT) ? 1 : 0, 0, TESTID_PRIV_FLOW + 5);

    /* Install the ACR1 timer handler and schedule a timer interrupt. */
    hl_ssrset_uimm24(SSR_EVBASE_ACR1, (uint64_t)(uintptr_t)&linx_acr1_timer_handler);
    uint64_t now = ssrget_time_symbol();
    hl_ssrset_uimm24(SSR_TIMER_TIMECMP_ACR1, now + 1000000ull); /* +1ms */

    /*
     * Wait until the timer interrupt is delivered.
     *
     * The interrupt handler returns directly to `linx_priv_after_irq` by
     * setting EBARG_BPC_CUR_ACR1 from ETEMP_ACR1.
     */
    const uint64_t deadline = ssrget_time_symbol() + 20000000ull; /* 20ms */
    while (ssrget_time_symbol() < deadline) {
        /* spin */
    }

    test_fail(TESTID_PRIV_FLOW + 2, 1, ssrget_uimm(SSR_IRQ_SEEN));
    __builtin_unreachable();
}

__attribute__((noreturn)) static void linx_priv_after_irq(void)
{
    TEST_EQ64(ssrget_uimm(SSR_IRQ_SEEN), 1, TESTID_PRIV_FLOW + 3);

    /* Switch ACR0 vector to the exit handler, then request a service exit. */
    ssrset_uimm(SSR_EVBASE_ACR0, (uint64_t)(uintptr_t)&linx_acr0_exit_handler);
    __asm__ volatile("acrc 0\n  c.bstop\n" : : : "memory"); /* SCT_MAC -> routes to ACR0 */
    __builtin_unreachable();
}

__attribute__((noreturn)) static void linx_priv_after_exit(void)
{
    test_pass(); /* PRIV_FLOW */

    /* --------------------------------------------------------------------- */
    /* ACRC adjacency negative test                                           */
    /* --------------------------------------------------------------------- */
    test_start(TESTID_ACRC_ADJ);

    ssrset_uimm(SSR_LAST_TRAPNO, 0);
    ssrset_uimm(SSR_LAST_TRAPARG0, 0);
    ssrset_uimm(SSR_LAST_EBARG_TPC, 0);
    hl_ssrset_uimm24(SSR_ETEMP0_ACR1, (uint64_t)(uintptr_t)&linx_trap_resume_to_exit);
    ssrset_uimm(SSR_CONT_EXIT, (uint64_t)(uintptr_t)&linx_after_bad_acrc_exit);

    /* Install handlers for v0.2-style trap recording + exit routing. */
    hl_ssrset_uimm24(SSR_EVBASE_ACR1, (uint64_t)(uintptr_t)&linx_acr1_record_trap_handler);
    ssrset_uimm(SSR_EVBASE_ACR0, (uint64_t)(uintptr_t)&linx_acr0_exit_handler);

    /* Enter ACR2 at the bad ACRC block; expect a BLOCK_TRAP in ACR1. */
    ssrset_uimm(SSR_ECSTATE_ACR0, 2); /* target ACR2 */
    ssrset_uimm(SSR_EBARG_BPC_CUR_ACR0, (uint64_t)(uintptr_t)&linx_bad_acrc_user);
    __asm__ volatile("acre 0" : : : "memory");
    __builtin_unreachable();
}

__attribute__((noreturn)) static void linx_after_bad_acrc_exit(void)
{
    const uint64_t trapno = ssrget_uimm(SSR_LAST_TRAPNO);
    const uint64_t trapnum = trapno_trapnum(trapno);

    TEST_EQ64(trapno_is_async(trapno), 0, TESTID_ACRC_ADJ + 1);
    TEST_EQ64(trapnum, 5 /* BLOCK_TRAP */, TESTID_ACRC_ADJ + 3);
    (void)trapno_cause(trapno);

    test_pass(); /* ACRC_ADJ */

    /* --------------------------------------------------------------------- */
    /* Hardware breakpoint trap (v0.2)                                        */
    /* --------------------------------------------------------------------- */
    test_start(TESTID_DBG_BP);

    ssrset_uimm(SSR_LAST_TRAPNO, 0);
    ssrset_uimm(SSR_LAST_TRAPARG0, 0);
    ssrset_uimm(SSR_LAST_EBARG_TPC, 0);
    hl_ssrset_uimm24(SSR_ETEMP0_ACR1, (uint64_t)(uintptr_t)&linx_trap_resume_to_exit);
    ssrset_uimm(SSR_CONT_EXIT, (uint64_t)(uintptr_t)&linx_after_dbg_bp_exit);

    /* Program BP0 (address match exact, no mask/linking). */
    hl_ssrset_uimm24(SSR_DBCR0_ACR2, 0); /* clear */
    hl_ssrset_uimm24(SSR_DBVR0_ACR2, 0);

    const uint64_t bp_pc = (uint64_t)(uintptr_t)&linx_dbg_bp_user + 2; /* skip C.BSTART (16-bit) */
    hl_ssrset_uimm24(SSR_DBVR0_ACR2, bp_pc);
    hl_ssrset_uimm24(SSR_DBCR0_ACR2, 1ull); /* E=1 */

    hl_ssrset_uimm24(SSR_EVBASE_ACR1, (uint64_t)(uintptr_t)&linx_acr1_record_trap_handler);
    ssrset_uimm(SSR_EVBASE_ACR0, (uint64_t)(uintptr_t)&linx_acr0_exit_handler);

    ssrset_uimm(SSR_ECSTATE_ACR0, 2); /* target ACR2 */
    ssrset_uimm(SSR_EBARG_BPC_CUR_ACR0, (uint64_t)(uintptr_t)&linx_dbg_bp_user);
    __asm__ volatile("acre 0" : : : "memory");
    __builtin_unreachable();
}

__attribute__((noreturn)) static void linx_after_dbg_bp_exit(void)
{
    const uint64_t trapno = ssrget_uimm(SSR_LAST_TRAPNO);
    const uint64_t traparg0 = ssrget_uimm(SSR_LAST_TRAPARG0);
    const uint64_t ebarg_tpc = ssrget_uimm(SSR_LAST_EBARG_TPC);

    const uint64_t bp_pc = (uint64_t)(uintptr_t)&linx_dbg_bp_user + 2;

    TEST_EQ64(trapno_is_async(trapno), 0, TESTID_DBG_BP + 1);
    TEST_EQ64(trapno_has_argv(trapno), 1, TESTID_DBG_BP + 2);
    TEST_EQ64(trapno_trapnum(trapno), 49 /* HW_BREAKPOINT */, TESTID_DBG_BP + 3);
    TEST_EQ64(traparg0, bp_pc, TESTID_DBG_BP + 4);
    TEST_EQ64(ebarg_tpc, bp_pc + 4, TESTID_DBG_BP + 5); /* trap resumes at next PC */

    /* Disable BP0. */
    hl_ssrset_uimm24(SSR_DBCR0_ACR2, 0);

    test_pass(); /* DBG_BP */

    /* --------------------------------------------------------------------- */
    /* Hardware breakpoint resume path (captured EBARG.TPC)                   */
    /* --------------------------------------------------------------------- */
    test_start(TESTID_DBG_BP_RESUME);

    ssrset_uimm(SSR_LAST_TRAPNO, 0);
    ssrset_uimm(SSR_LAST_TRAPARG0, 0);
    ssrset_uimm(SSR_LAST_EBARG_TPC, 0);
    ssrset_uimm(SSR_LAST_ECSTATE, 0);
    ssrset_uimm(SSR_BP_RESUME_SEEN, 0);
    ssrset_uimm(SSR_ACR0_TRAPNO, 0);
    ssrset_uimm(SSR_ACR0_TRAPARG0, 0);
    ssrset_uimm(SSR_ACR0_ECSTATE, 0);
    ssrset_uimm(SSR_CONT_EXIT, (uint64_t)(uintptr_t)&linx_after_dbg_bp_resume_exit);

    hl_ssrset_uimm24(SSR_EVBASE_ACR1, (uint64_t)(uintptr_t)&linx_acr1_bp_resume_handler);
    ssrset_uimm(SSR_EVBASE_ACR0, (uint64_t)(uintptr_t)&linx_acr0_exit_handler);

    hl_ssrset_uimm24(SSR_DBCR0_ACR2, 0); /* clear */
    hl_ssrset_uimm24(SSR_DBVR0_ACR2, 0);

    const uint64_t bp_resume_pc = (uint64_t)(uintptr_t)&linx_dbg_bp_resume_user + 2;
    hl_ssrset_uimm24(SSR_DBVR0_ACR2, bp_resume_pc);
    hl_ssrset_uimm24(SSR_DBCR0_ACR2, 1ull); /* E=1 */

    ssrset_uimm(SSR_ECSTATE_ACR0, 2); /* target ACR2 */
    ssrset_uimm(SSR_EBARG_BPC_CUR_ACR0, (uint64_t)(uintptr_t)&linx_dbg_bp_resume_user);
    __asm__ volatile("acre 0" : : : "memory");
    __builtin_unreachable();
}

__attribute__((noreturn)) static void linx_after_dbg_bp_resume_exit(void)
{
    const uint64_t trapno = ssrget_uimm(SSR_LAST_TRAPNO);
    const uint64_t traparg0 = ssrget_uimm(SSR_LAST_TRAPARG0);
    const uint64_t ebarg_tpc = ssrget_uimm(SSR_LAST_EBARG_TPC);
    const uint64_t ecstate = ssrget_uimm(SSR_LAST_ECSTATE);
    const uint64_t acr0_trapno = ssrget_uimm(SSR_ACR0_TRAPNO);
    const uint64_t acr0_traparg0 = ssrget_uimm(SSR_ACR0_TRAPARG0);
    const uint64_t acr0_ecstate = ssrget_uimm(SSR_ACR0_ECSTATE);
    const uint64_t bp_pc = (uint64_t)(uintptr_t)&linx_dbg_bp_resume_user + 2;

    TEST_EQ64(trapno_is_async(trapno), 0, TESTID_DBG_BP_RESUME + 1);
    TEST_EQ64(trapno_has_argv(trapno), 1, TESTID_DBG_BP_RESUME + 2);
    TEST_EQ64(trapno_trapnum(trapno), 49 /* HW_BREAKPOINT */, TESTID_DBG_BP_RESUME + 3);
    TEST_EQ64(traparg0, bp_pc, TESTID_DBG_BP_RESUME + 4);
    TEST_EQ64(ebarg_tpc, bp_pc + 4, TESTID_DBG_BP_RESUME + 5);
    TEST_EQ64(ecstate & CSTATE_ACR_MASK, 2, TESTID_DBG_BP_RESUME + 6);
    TEST_EQ64(ssrget_uimm(SSR_BP_RESUME_SEEN), 1, TESTID_DBG_BP_RESUME + 7);
    TEST_EQ64(trapno_is_async(acr0_trapno), 0, TESTID_DBG_BP_RESUME + 8);
    TEST_EQ64(trapno_trapnum(acr0_trapno), 6 /* SYSCALL */, TESTID_DBG_BP_RESUME + 9);
    TEST_EQ64(acr0_traparg0, 0 /* SCT_MAC */, TESTID_DBG_BP_RESUME + 10);
    TEST_EQ64(acr0_ecstate & CSTATE_ACR_MASK, 2, TESTID_DBG_BP_RESUME + 11);

    /* Disable BP0. */
    hl_ssrset_uimm24(SSR_DBCR0_ACR2, 0);

    test_pass(); /* DBG_BP_RESUME */

    /* --------------------------------------------------------------------- */
    /* Hardware watchpoint trap (v0.2)                                        */
    /* --------------------------------------------------------------------- */
    test_start(TESTID_DBG_WP);

    ssrset_uimm(SSR_LAST_TRAPNO, 0);
    ssrset_uimm(SSR_LAST_TRAPARG0, 0);
    ssrset_uimm(SSR_LAST_EBARG_TPC, 0);
    hl_ssrset_uimm24(SSR_ETEMP0_ACR1, (uint64_t)(uintptr_t)&linx_trap_resume_to_exit);
    ssrset_uimm(SSR_CONT_EXIT, (uint64_t)(uintptr_t)&linx_after_dbg_wp_exit);

    const uint64_t wp_addr = (uint64_t)(uintptr_t)&WATCH_TARGET;
    hl_ssrset_uimm24(SSR_DWCR0_ACR2, 0);
    hl_ssrset_uimm24(SSR_DWVR0_ACR2, 0);
    hl_ssrset_uimm24(SSR_DWVR0_ACR2, wp_addr);
    hl_ssrset_uimm24(SSR_DWCR0_ACR2, (1ull << 0) | (2ull << 4)); /* E=1, LS=store */

    hl_ssrset_uimm24(SSR_EVBASE_ACR1, (uint64_t)(uintptr_t)&linx_acr1_record_trap_handler);
    ssrset_uimm(SSR_EVBASE_ACR0, (uint64_t)(uintptr_t)&linx_acr0_exit_handler);

    ssrset_uimm(SSR_ECSTATE_ACR0, 2); /* target ACR2 */
    ssrset_uimm(SSR_EBARG_BPC_CUR_ACR0, (uint64_t)(uintptr_t)&linx_dbg_wp_user);
    __asm__ volatile("acre 0" : : : "memory");
    __builtin_unreachable();
}

__attribute__((noreturn)) static void linx_after_dbg_wp_exit(void)
{
    const uint64_t trapno = ssrget_uimm(SSR_LAST_TRAPNO);
    const uint64_t traparg0 = ssrget_uimm(SSR_LAST_TRAPARG0);

    const uint64_t wp_addr = (uint64_t)(uintptr_t)&WATCH_TARGET;

    TEST_EQ64(trapno_is_async(trapno), 0, TESTID_DBG_WP + 1);
    TEST_EQ64(trapno_has_argv(trapno), 1, TESTID_DBG_WP + 2);
    TEST_EQ64(trapno_trapnum(trapno), 51 /* HW_WATCHPOINT */, TESTID_DBG_WP + 3);
    TEST_EQ64(traparg0, wp_addr, TESTID_DBG_WP + 4);

    /* Disable WP0. */
    hl_ssrset_uimm24(SSR_DWCR0_ACR2, 0);

    test_pass(); /* DBG_WP */

    /* --------------------------------------------------------------------- */
    /* ACR routing matrix: ACR2(SCT_MAC)->ACR0                               */
    /* --------------------------------------------------------------------- */
    test_start(TESTID_ACR_ROUTE_A2_MAC);

    ssrset_uimm(SSR_ACR0_TRAPNO, 0);
    ssrset_uimm(SSR_ACR0_TRAPARG0, 0);
    ssrset_uimm(SSR_ACR0_ECSTATE, 0);
    ssrset_uimm(SSR_CONT_EXIT, (uint64_t)(uintptr_t)&linx_after_acr2_mac_exit);
    ssrset_uimm(SSR_EVBASE_ACR0, (uint64_t)(uintptr_t)&linx_acr0_exit_handler);
    ssrset_uimm(SSR_ECSTATE_ACR0, 2); /* enter ACR2 */
    ssrset_uimm(SSR_EBARG_BPC_CUR_ACR0, (uint64_t)(uintptr_t)&linx_acr2_mac_user);
    __asm__ volatile("acre 0" : : : "memory");
    __builtin_unreachable();
}

__attribute__((noreturn)) static void linx_after_acr2_mac_exit(void)
{
    const uint64_t trapno = ssrget_uimm(SSR_ACR0_TRAPNO);
    const uint64_t traparg0 = ssrget_uimm(SSR_ACR0_TRAPARG0);
    const uint64_t ecstate = ssrget_uimm(SSR_ACR0_ECSTATE);

    TEST_EQ64(trapno_is_async(trapno), 0, TESTID_ACR_ROUTE_A2_MAC + 1);
    TEST_EQ64(trapno_trapnum(trapno), 6 /* SYSCALL */, TESTID_ACR_ROUTE_A2_MAC + 2);
    TEST_EQ64(traparg0, 0 /* SCT_MAC */, TESTID_ACR_ROUTE_A2_MAC + 3);
    TEST_EQ64(ecstate & CSTATE_ACR_MASK, 2, TESTID_ACR_ROUTE_A2_MAC + 4);
    TEST_EQ64(trapno_cause(trapno), 0, TESTID_ACR_ROUTE_A2_MAC + 5);

    test_pass();

    /* --------------------------------------------------------------------- */
    /* ACR routing matrix: ACR1(SCT_SEC)->ACR0                               */
    /* --------------------------------------------------------------------- */
    test_start(TESTID_ACR_ROUTE_A1_SEC);

    ssrset_uimm(SSR_ACR0_TRAPNO, 0);
    ssrset_uimm(SSR_ACR0_TRAPARG0, 0);
    ssrset_uimm(SSR_ACR0_ECSTATE, 0);
    ssrset_uimm(SSR_CONT_EXIT, (uint64_t)(uintptr_t)&linx_after_acr1_sec_exit);
    ssrset_uimm(SSR_EVBASE_ACR0, (uint64_t)(uintptr_t)&linx_acr0_exit_handler);
    ssrset_uimm(SSR_ECSTATE_ACR0, 1); /* enter ACR1 */
    ssrset_uimm(SSR_EBARG_BPC_CUR_ACR0, (uint64_t)(uintptr_t)&linx_acr1_sec_user);
    __asm__ volatile("acre 0" : : : "memory");
    __builtin_unreachable();
}

__attribute__((noreturn)) static void linx_after_acr1_sec_exit(void)
{
    const uint64_t trapno = ssrget_uimm(SSR_ACR0_TRAPNO);
    const uint64_t traparg0 = ssrget_uimm(SSR_ACR0_TRAPARG0);
    const uint64_t ecstate = ssrget_uimm(SSR_ACR0_ECSTATE);

    TEST_EQ64(trapno_is_async(trapno), 0, TESTID_ACR_ROUTE_A1_SEC + 1);
    TEST_EQ64(trapno_trapnum(trapno), 6 /* SYSCALL */, TESTID_ACR_ROUTE_A1_SEC + 2);
    TEST_EQ64(traparg0, 2 /* SCT_SEC */, TESTID_ACR_ROUTE_A1_SEC + 3);
    TEST_EQ64(ecstate & CSTATE_ACR_MASK, 1, TESTID_ACR_ROUTE_A1_SEC + 4);
    TEST_EQ64(trapno_cause(trapno), 2, TESTID_ACR_ROUTE_A1_SEC + 5);

    test_pass();

    /* --------------------------------------------------------------------- */
    /* IRQ gate in ACR1: I=0 blocks same-ring delivery until enabled         */
    /* --------------------------------------------------------------------- */
    test_start(TESTID_IRQ_GATE_ACR1);

    ssrset_uimm(SSR_IRQ_SEEN, 0);
    ssrset_uimm(SSR_IRQ_SEEN_BEFORE_ENABLE, 0);
    ssrset_uimm(SSR_CONT_EXIT, (uint64_t)(uintptr_t)&linx_after_irq_gate_exit);
    hl_ssrset_uimm24(SSR_EVBASE_ACR1, (uint64_t)(uintptr_t)&linx_acr1_timer_handler);
    ssrset_uimm(SSR_ECSTATE_ACR0, 1); /* enter ACR1 */
    ssrset_uimm(SSR_EBARG_BPC_CUR_ACR0, (uint64_t)(uintptr_t)&linx_acr1_irq_gate_user);
    __asm__ volatile("acre 0" : : : "memory");
    __builtin_unreachable();
}

__attribute__((noreturn)) static void linx_acr1_irq_gate_user(void)
{
    uint64_t cstate = ssrget_cstate_symbol();
    const uint64_t now = ssrget_time_symbol();
    const uint64_t block_deadline = now + 5000000ull;  /* 5ms */
    const uint64_t fail_deadline = now + 25000000ull;  /* 25ms */

    TEST_EQ64(cstate & CSTATE_ACR_MASK, 1, TESTID_IRQ_GATE_ACR1 + 1);

    hl_ssrset_uimm24(SSR_ETEMP_ACR1, (uint64_t)(uintptr_t)&linx_acr1_irq_gate_after);

    /* Same-ring IRQ should stay pending while CSTATE.I=0. */
    cstate &= ~CSTATE_I_BIT;
    ssrset_cstate_symbol(cstate);
    hl_ssrset_uimm24(SSR_TIMER_TIMECMP_ACR1, now + 1000000ull); /* +1ms */

    while (ssrget_time_symbol() < block_deadline) {
        /* wait for timer to become pending */
    }
    ssrset_uimm(SSR_IRQ_SEEN_BEFORE_ENABLE, ssrget_uimm(SSR_IRQ_SEEN));
    TEST_EQ64(ssrget_uimm(SSR_IRQ_SEEN), 0, TESTID_IRQ_GATE_ACR1 + 2);

    /* Enable CSTATE.I; pending IRQ should now be delivered. */
    cstate |= CSTATE_I_BIT;
    ssrset_cstate_symbol(cstate);

    while (ssrget_time_symbol() < fail_deadline) {
        /* interrupt handler should redirect control to linx_acr1_irq_gate_after */
    }

    test_fail(TESTID_IRQ_GATE_ACR1 + 3, 1, ssrget_uimm(SSR_IRQ_SEEN));
    __builtin_unreachable();
}

__attribute__((noreturn)) static void linx_acr1_irq_gate_after(void)
{
    TEST_EQ64(ssrget_uimm(SSR_IRQ_SEEN_BEFORE_ENABLE), 0, TESTID_IRQ_GATE_ACR1 + 4);
    TEST_EQ64(ssrget_uimm(SSR_IRQ_SEEN), 1, TESTID_IRQ_GATE_ACR1 + 5);

    /* Exit back to ACR0 for final verification/reporting. */
    __asm__ volatile("acrc 0\n  c.bstop\n" : : : "memory");
    __builtin_unreachable();
}

__attribute__((noreturn)) static void linx_after_irq_gate_exit(void)
{
    const uint64_t trapno = ssrget_uimm(SSR_ACR0_TRAPNO);
    const uint64_t traparg0 = ssrget_uimm(SSR_ACR0_TRAPARG0);
    const uint64_t ecstate = ssrget_uimm(SSR_ACR0_ECSTATE);

    TEST_EQ64(trapno_is_async(trapno), 0, TESTID_IRQ_GATE_ACR1 + 6);
    TEST_EQ64(trapno_trapnum(trapno), 6 /* SYSCALL */, TESTID_IRQ_GATE_ACR1 + 7);
    TEST_EQ64(traparg0, 0 /* SCT_MAC */, TESTID_IRQ_GATE_ACR1 + 8);
    TEST_EQ64(ecstate & CSTATE_ACR_MASK, 1, TESTID_IRQ_GATE_ACR1 + 9);

    test_pass();

    /* --------------------------------------------------------------------- */
    /* ACR1 request validation: SCT_SYS from ACR1 must trap illegal          */
    /* --------------------------------------------------------------------- */
    test_start(TESTID_ACR1_BAD_REQ);

    ssrset_uimm(SSR_LAST_TRAPNO, 0);
    ssrset_uimm(SSR_LAST_TRAPARG0, 0);
    ssrset_uimm(SSR_LAST_EBARG_TPC, 0);
    hl_ssrset_uimm24(SSR_ETEMP0_ACR1, (uint64_t)(uintptr_t)&linx_after_acr1_bad_req_trap);
    ssrset_uimm(SSR_CONT_EXIT, (uint64_t)(uintptr_t)&linx_after_acr1_bad_req_exit);
    hl_ssrset_uimm24(SSR_EVBASE_ACR1, (uint64_t)(uintptr_t)&linx_acr1_record_trap_handler);
    ssrset_uimm(SSR_EVBASE_ACR0, (uint64_t)(uintptr_t)&linx_acr0_exit_handler);
    ssrset_uimm(SSR_ECSTATE_ACR0, 1); /* enter ACR1 */
    ssrset_uimm(SSR_EBARG_BPC_CUR_ACR0, (uint64_t)(uintptr_t)&linx_acr1_bad_req_user);
    __asm__ volatile("acre 0" : : : "memory");
    __builtin_unreachable();
}

__attribute__((noreturn)) static void linx_after_acr1_bad_req_trap(void)
{
    const uint64_t trapno = ssrget_uimm(SSR_LAST_TRAPNO);

    TEST_EQ64(trapno_is_async(trapno), 0, TESTID_ACR1_BAD_REQ + 1);
    TEST_EQ64(trapno_trapnum(trapno), 4 /* ILLEGAL_INST */, TESTID_ACR1_BAD_REQ + 2);

    /* Exit back to ACR0 after validating trap class. */
    __asm__ volatile("acrc 0\n  c.bstop\n" : : : "memory");
    __builtin_unreachable();
}

__attribute__((noreturn)) static void linx_after_acr1_bad_req_exit(void)
{
    const uint64_t trapno = ssrget_uimm(SSR_ACR0_TRAPNO);
    const uint64_t traparg0 = ssrget_uimm(SSR_ACR0_TRAPARG0);
    const uint64_t ecstate = ssrget_uimm(SSR_ACR0_ECSTATE);

    TEST_EQ64(trapno_is_async(trapno), 0, TESTID_ACR1_BAD_REQ + 3);
    TEST_EQ64(trapno_trapnum(trapno), 6 /* SYSCALL */, TESTID_ACR1_BAD_REQ + 4);
    TEST_EQ64(traparg0, 0 /* SCT_MAC */, TESTID_ACR1_BAD_REQ + 5);
    TEST_EQ64(ecstate & CSTATE_ACR_MASK, 1, TESTID_ACR1_BAD_REQ + 6);

    test_pass();

    /* --------------------------------------------------------------------- */
    /* Cross-ring IRQ preemption: ACR2 I=0 still preempts to ACR1           */
    /* --------------------------------------------------------------------- */
    test_start(TESTID_IRQ_PREEMPT_A2);

    ssrset_uimm(SSR_IRQ_SEEN, 0);
    ssrset_uimm(SSR_CONT_EXIT, (uint64_t)(uintptr_t)&linx_after_acr2_irq_preempt_exit);
    hl_ssrset_uimm24(SSR_EVBASE_ACR1, (uint64_t)(uintptr_t)&linx_acr1_timer_handler);
    ssrset_uimm(SSR_ECSTATE_ACR0, 2); /* enter ACR2 */
    ssrset_uimm(SSR_EBARG_BPC_CUR_ACR0, (uint64_t)(uintptr_t)&linx_acr2_irq_preempt_user);
    __asm__ volatile("acre 0" : : : "memory");
    __builtin_unreachable();
}

__attribute__((noreturn)) static void linx_acr2_irq_preempt_user(void)
{
    uint64_t cstate = ssrget_cstate_symbol();
    const uint64_t now = ssrget_time_symbol();
    const uint64_t fail_deadline = now + 25000000ull; /* 25ms */

    TEST_EQ64(cstate & CSTATE_ACR_MASK, 2, TESTID_IRQ_PREEMPT_A2 + 1);

    /* Timer IRQ handler returns directly here via ETEMP_ACR1. */
    hl_ssrset_uimm24(SSR_ETEMP_ACR1, (uint64_t)(uintptr_t)&linx_acr2_irq_preempt_after);

    /* Keep same-ring interrupts masked; cross-ring delivery must still happen. */
    cstate &= ~CSTATE_I_BIT;
    ssrset_cstate_symbol(cstate);
    hl_ssrset_uimm24(SSR_TIMER_TIMECMP_ACR1, now + 1000000ull); /* +1ms */

    while (ssrget_time_symbol() < fail_deadline) {
        /* wait for IRQ preemption to redirect control */
    }

    test_fail(TESTID_IRQ_PREEMPT_A2 + 2, 1, ssrget_uimm(SSR_IRQ_SEEN));
    __builtin_unreachable();
}

__attribute__((noreturn)) static void linx_acr2_irq_preempt_after(void)
{
    TEST_EQ64(ssrget_uimm(SSR_IRQ_SEEN), 1, TESTID_IRQ_PREEMPT_A2 + 3);
    TEST_EQ64(ssrget_cstate_symbol() & CSTATE_ACR_MASK, 2, TESTID_IRQ_PREEMPT_A2 + 4);

    __asm__ volatile("acrc 0\n  c.bstop\n" : : : "memory");
    __builtin_unreachable();
}

__attribute__((noreturn)) static void linx_after_acr2_irq_preempt_exit(void)
{
    const uint64_t trapno = ssrget_uimm(SSR_ACR0_TRAPNO);
    const uint64_t traparg0 = ssrget_uimm(SSR_ACR0_TRAPARG0);
    const uint64_t ecstate = ssrget_uimm(SSR_ACR0_ECSTATE);

    TEST_EQ64(trapno_is_async(trapno), 0, TESTID_IRQ_PREEMPT_A2 + 5);
    TEST_EQ64(trapno_trapnum(trapno), 6 /* SYSCALL */, TESTID_IRQ_PREEMPT_A2 + 6);
    TEST_EQ64(traparg0, 0 /* SCT_MAC */, TESTID_IRQ_PREEMPT_A2 + 7);
    TEST_EQ64(ecstate & CSTATE_ACR_MASK, 2, TESTID_IRQ_PREEMPT_A2 + 8);

    test_pass();

    /* --------------------------------------------------------------------- */
    /* IRQ metadata from ACR2: async trap encoding must be v0.2-consistent   */
    /* --------------------------------------------------------------------- */
    test_start(TESTID_IRQ_META_A2);

    ssrset_uimm(SSR_LAST_TRAPNO, 0);
    ssrset_uimm(SSR_LAST_TRAPARG0, 0);
    ssrset_uimm(SSR_LAST_EBARG_TPC, 0);
    ssrset_uimm(SSR_LAST_ECSTATE, 0);
    hl_ssrset_uimm24(SSR_ETEMP0_ACR1, (uint64_t)(uintptr_t)&linx_acr2_irq_meta_after);
    ssrset_uimm(SSR_CONT_EXIT, (uint64_t)(uintptr_t)&linx_after_irq_meta_exit);
    hl_ssrset_uimm24(SSR_EVBASE_ACR1, (uint64_t)(uintptr_t)&linx_acr1_record_trap_handler);
    ssrset_uimm(SSR_EVBASE_ACR0, (uint64_t)(uintptr_t)&linx_acr0_exit_handler);
    ssrset_uimm(SSR_ECSTATE_ACR0, 2); /* enter ACR2 */
    ssrset_uimm(SSR_EBARG_BPC_CUR_ACR0, (uint64_t)(uintptr_t)&linx_acr2_irq_meta_user);
    __asm__ volatile("acre 0" : : : "memory");
    __builtin_unreachable();
}

__attribute__((noreturn)) static void linx_acr2_irq_meta_user(void)
{
    uint64_t cstate = ssrget_cstate_symbol();
    const uint64_t now = ssrget_time_symbol();
    const uint64_t fail_deadline = now + 25000000ull; /* 25ms */

    TEST_EQ64(cstate & CSTATE_ACR_MASK, 2, TESTID_IRQ_META_A2 + 1);

    cstate &= ~CSTATE_I_BIT;
    ssrset_cstate_symbol(cstate);
    hl_ssrset_uimm24(SSR_TIMER_TIMECMP_ACR1, now + 1000000ull); /* +1ms */

    while (ssrget_time_symbol() < fail_deadline) {
        /* wait for IRQ preemption to redirect to linx_acr2_irq_meta_after */
    }

    test_fail(TESTID_IRQ_META_A2 + 2, 1, 0);
    __builtin_unreachable();
}

__attribute__((noreturn)) static void linx_acr2_irq_meta_after(void)
{
    const uint64_t trapno = ssrget_uimm(SSR_LAST_TRAPNO);
    const uint64_t traparg0 = ssrget_uimm(SSR_LAST_TRAPARG0);
    const uint64_t ebarg_tpc = ssrget_uimm(SSR_LAST_EBARG_TPC);
    const uint64_t ecstate = ssrget_uimm(SSR_LAST_ECSTATE);

    TEST_EQ64(trapno_is_async(trapno), 1, TESTID_IRQ_META_A2 + 3);
    TEST_EQ64(trapno_has_argv(trapno), 1, TESTID_IRQ_META_A2 + 4);
    TEST_EQ64(trapno_trapnum(trapno), 44 /* INTERRUPT */, TESTID_IRQ_META_A2 + 5);
    TEST_EQ64(trapno_cause(trapno), 0, TESTID_IRQ_META_A2 + 6);
    TEST_EQ64(traparg0, 0 /* irq_id(timer0) */, TESTID_IRQ_META_A2 + 7);
    TEST_EQ64(ecstate & CSTATE_ACR_MASK, 2, TESTID_IRQ_META_A2 + 8);
    TEST_ASSERT(ebarg_tpc != 0, TESTID_IRQ_META_A2 + 9, 1, ebarg_tpc);

    __asm__ volatile("acrc 0\n  c.bstop\n" : : : "memory");
    __builtin_unreachable();
}

__attribute__((noreturn)) static void linx_after_irq_meta_exit(void)
{
    const uint64_t trapno = ssrget_uimm(SSR_ACR0_TRAPNO);
    const uint64_t traparg0 = ssrget_uimm(SSR_ACR0_TRAPARG0);
    const uint64_t ecstate = ssrget_uimm(SSR_ACR0_ECSTATE);

    TEST_EQ64(trapno_is_async(trapno), 0, TESTID_IRQ_META_A2 + 10);
    TEST_EQ64(trapno_trapnum(trapno), 6 /* SYSCALL */, TESTID_IRQ_META_A2 + 11);
    TEST_EQ64(traparg0, 0 /* SCT_MAC */, TESTID_IRQ_META_A2 + 12);
    TEST_EQ64(ecstate & CSTATE_ACR_MASK, 2, TESTID_IRQ_META_A2 + 13);

    test_pass();

    /* --------------------------------------------------------------------- */
    /* ACRE target validation: ACR1 -> ACR0 must trap EXEC_STATE_CHECK       */
    /* --------------------------------------------------------------------- */
    test_start(TESTID_ACRE_BAD_TARGET);

    ssrset_uimm(SSR_LAST_TRAPNO, 0);
    ssrset_uimm(SSR_LAST_TRAPARG0, 0);
    ssrset_uimm(SSR_LAST_EBARG_TPC, 0);
    ssrset_uimm(SSR_LAST_ECSTATE, 0);
    hl_ssrset_uimm24(SSR_ETEMP0_ACR1, (uint64_t)(uintptr_t)&linx_after_acr1_bad_target_trap);
    ssrset_uimm(SSR_CONT_EXIT, (uint64_t)(uintptr_t)&linx_after_acr1_bad_target_exit);
    hl_ssrset_uimm24(SSR_EVBASE_ACR1, (uint64_t)(uintptr_t)&linx_acr1_record_trap_handler);
    ssrset_uimm(SSR_EVBASE_ACR0, (uint64_t)(uintptr_t)&linx_acr0_exit_handler);
    ssrset_uimm(SSR_ECSTATE_ACR0, 1); /* enter ACR1 */
    ssrset_uimm(SSR_EBARG_BPC_CUR_ACR0, (uint64_t)(uintptr_t)&linx_acr1_bad_target_user);
    __asm__ volatile("acre 0" : : : "memory");
    __builtin_unreachable();
}

__attribute__((noreturn)) static void linx_after_acr1_bad_target_trap(void)
{
    const uint64_t trapno = ssrget_uimm(SSR_LAST_TRAPNO);
    const uint64_t traparg0 = ssrget_uimm(SSR_LAST_TRAPARG0);
    const uint64_t trapnum = trapno_trapnum(trapno);
    const uint64_t argv = trapno_has_argv(trapno);

    TEST_EQ64(trapno_is_async(trapno), 0, TESTID_ACRE_BAD_TARGET + 1);
    /*
     * Bring-up compatibility: some QEMU lanes now tag EXEC_STATE_CHECK with
     * ARGV=1 and provide TRAPARG0 (target ring), while older lanes use ARGV=0.
     */
    TEST_ASSERT(argv == 0 || argv == 1, TESTID_ACRE_BAD_TARGET + 2, 1, argv);
    /*
     * Older lanes report EXEC_STATE_CHECK directly; newer lanes can surface
     * BAD_BRANCH_TARGET (cause=1) when the invalid ACRE target path is
     * materialized through the block-target validator.
     */
    TEST_ASSERT(trapnum == 0 || trapnum == 5, TESTID_ACRE_BAD_TARGET + 3, 1, trapnum);
    if (trapnum == 5) {
        TEST_EQ64(trapno_cause(trapno), 1 /* BAD_BRANCH_TARGET */, TESTID_ACRE_BAD_TARGET + 9);
    }
    if (argv == 1) {
        TEST_EQ64(traparg0, 0 /* invalid ACRE target ACR0 */, TESTID_ACRE_BAD_TARGET + 8);
    }

    /*
     * Resume path: exit ACR1 back to ACR0 through a direct syscall request.
     * Using a local ACRC sequence avoids recursive bad-target trap loops seen
     * on some QEMU lanes when resuming through the helper block.
     */
    __asm__ volatile("acrc 0\n  c.bstop\n" : : : "memory");
    __builtin_unreachable();
}

__attribute__((noreturn)) static void linx_after_acr1_bad_target_exit(void)
{
    const uint64_t trapno = ssrget_uimm(SSR_ACR0_TRAPNO);
    const uint64_t traparg0 = ssrget_uimm(SSR_ACR0_TRAPARG0);
    const uint64_t ecstate = ssrget_uimm(SSR_ACR0_ECSTATE);

    TEST_EQ64(trapno_is_async(trapno), 0, TESTID_ACRE_BAD_TARGET + 4);
    TEST_EQ64(trapno_trapnum(trapno), 6 /* SYSCALL */, TESTID_ACRE_BAD_TARGET + 5);
    TEST_EQ64(traparg0, 0 /* SCT_MAC */, TESTID_ACRE_BAD_TARGET + 6);
    TEST_EQ64(ecstate & CSTATE_ACR_MASK, 1, TESTID_ACRE_BAD_TARGET + 7);

    test_pass();

    /* --------------------------------------------------------------------- */
    /* ACR0 privilege check: ACRC is illegal in ACR0                         */
    /* --------------------------------------------------------------------- */
    test_start(TESTID_ACR0_BAD_REQ);

    ssrset_uimm(SSR_ACR0_TRAPNO, 0);
    ssrset_uimm(SSR_ACR0_TRAPARG0, 0);
    ssrset_uimm(SSR_ACR0_ECSTATE, 0);
    ssrset_uimm(SSR_CONT_EXIT, (uint64_t)(uintptr_t)&linx_after_acr0_bad_req_exit);
    ssrset_uimm(SSR_EVBASE_ACR0, (uint64_t)(uintptr_t)&linx_acr0_exit_handler);
    __asm__ volatile("acrc 0\n  c.bstop\n" : : : "memory");
    __builtin_unreachable();
}

__attribute__((noreturn)) static void linx_after_acr0_bad_req_exit(void)
{
    const uint64_t trapno = ssrget_uimm(SSR_ACR0_TRAPNO);
    const uint64_t ecstate = ssrget_uimm(SSR_ACR0_ECSTATE);

    TEST_EQ64(trapno_is_async(trapno), 0, TESTID_ACR0_BAD_REQ + 1);
    TEST_EQ64(trapno_trapnum(trapno), 4 /* ILLEGAL_INST */, TESTID_ACR0_BAD_REQ + 2);
    TEST_EQ64(ecstate & CSTATE_ACR_MASK, 0, TESTID_ACR0_BAD_REQ + 3);

    test_pass();
    /*
     * Finish in-place instead of tail-calling another helper to avoid
     * block-target validation ambiguity on strict bring-up lanes.
     */
    uart_puts("*** REGRESSION PASSED ***\r\n");
    EXIT_CODE = 0;
    while (1) {
        /* Exit register should terminate QEMU; keep a safe hard-stop loop. */
    }
}

__attribute__((noreturn)) static void linx_system_done(void)
{
    uart_puts("*** REGRESSION PASSED ***\r\n");
    EXIT_CODE = 0;
    while (1) {
        /* If QEMU doesn't exit for some reason, don't fall through. */
    }
}

void run_system_tests(void)
{
    test_suite_begin(0x53595354u); /* 'SYST' */

    /* --------------------------------------------------------------------- */
    /* Base SSR access + symbolic IDs                                         */
    /* --------------------------------------------------------------------- */
    test_start(TESTID_SSR_BASIC);

    ssrset_uimm(SSR_SCRATCH0, 0x1122334455667788ull);
    TEST_EQ64(ssrget_uimm(SSR_SCRATCH0), 0x1122334455667788ull, TESTID_SSR_BASIC);

    TEST_EQ64(ssrswap_uimm(SSR_SCRATCH0, 0xAABBCCDDEEFF0011ull),
              0x1122334455667788ull,
              TESTID_SSR_BASIC + 1);
    TEST_EQ64(ssrget_uimm(SSR_SCRATCH0), 0xAABBCCDDEEFF0011ull, TESTID_SSR_BASIC + 2);

    /* TIME should be monotonic. */
    uint64_t t0 = ssrget_time_symbol();
    for (volatile int i = 0; i < 1000; i++) {
        /* busy */
    }
    uint64_t t1 = ssrget_time_symbol();
    TEST_ASSERT(t1 >= t0, TESTID_SSR_BASIC + 3, t0, t1);

    /* CYCLE symbolic name must map to 0x0C00 (QEMU models as insn_count). */
    uint64_t c0 = ssrget_cycle_symbol();
    for (volatile int i = 0; i < 1000; i++) {
        /* busy */
    }
    uint64_t c1 = ssrget_cycle_symbol();
    TEST_ASSERT(c1 >= c0, TESTID_SSR_BASIC + 4, c0, c1);

    test_pass();

    /* --------------------------------------------------------------------- */
    /* HL.SSRGET/HL.SSRSET (extended IDs)                                     */
    /* --------------------------------------------------------------------- */
    test_start(TESTID_SSR_HL);

    /* Use an ACR1-only manager SSR ID to force HL forms (0x1F10). */
    hl_ssrset_uimm24(SSR_ETEMP0_ACR1, 0x55aa1234ull);
    TEST_EQ64(hl_ssrget_uimm24(SSR_ETEMP0_ACR1), 0x55aa1234ull, TESTID_SSR_HL);

    test_pass();

    /* --------------------------------------------------------------------- */
    /* Context switch + service request + timer interrupt                     */
    /* --------------------------------------------------------------------- */
    test_start(TESTID_PRIV_FLOW);

    /* Clear flags + publish continuation PCs for ACR1 handlers. */
    ssrset_uimm(SSR_SYSCALL_SEEN, 0);
    ssrset_uimm(SSR_IRQ_SEEN, 0);
    hl_ssrset_uimm24(SSR_ETEMP0_ACR1, (uint64_t)(uintptr_t)&linx_priv_after_syscall);
    hl_ssrset_uimm24(SSR_ETEMP_ACR1, (uint64_t)(uintptr_t)&linx_priv_after_irq);
    ssrset_uimm(SSR_CONT_EXIT, (uint64_t)(uintptr_t)&linx_priv_after_exit);

    /* Install handler vectors. */
    hl_ssrset_uimm24(SSR_EVBASE_ACR1, (uint64_t)(uintptr_t)&linx_acr1_syscall_handler);

    /* Hand off to ACR2 at the user-code stage function. */
    ssrset_uimm(SSR_ECSTATE_ACR0, 2); /* target ACR2 (low bits) */
    ssrset_uimm(SSR_EBARG_BPC_CUR_ACR0, (uint64_t)(uintptr_t)&linx_priv_user_code);
    __asm__ volatile("acre 0" : : : "memory");
    __builtin_unreachable();
}
