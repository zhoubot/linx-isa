// Force a non-tail noreturn call site and ensure it still carries a fused
// call header with an explicit return target.
__attribute__((noinline, noreturn)) static void callret_sink_noreturn(int x) {
  volatile int sink = x;
  (void)sink;
  for (;;) {
    __asm__ __volatile__("" ::: "memory");
  }
}

__attribute__((noinline, disable_tail_calls)) int callret_noreturn_entry(int x) {
  callret_sink_noreturn(x + 1);
}
