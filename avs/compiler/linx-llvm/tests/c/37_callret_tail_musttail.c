typedef long (*tail_fn)(long);

static __attribute__((noinline)) long tail_target(long x) { return x + 9; }
static volatile tail_fn g_tail_target = tail_target;

long callret_tail_direct(long x) {
  __attribute__((musttail)) return tail_target(x);
}

long callret_tail_indirect(long x) {
  tail_fn fn = g_tail_target;
  __attribute__((musttail)) return fn(x);
}
