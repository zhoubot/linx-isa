static __attribute__((noinline)) long add3(long x) { return x + 3; }
static __attribute__((noinline)) long sub2(long x) { return x - 2; }

long callret_direct(long x) {
  long a = add3(x);
  long b = sub2(a);
  return b + 1;
}
