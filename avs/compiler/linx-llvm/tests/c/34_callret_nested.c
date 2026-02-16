static __attribute__((noinline)) long leaf(long x) { return x * 7; }

static __attribute__((noinline)) long mid(long x) {
  long a = leaf(x + 1);
  long b = leaf(a - 3);
  return a ^ b;
}

long callret_nested(long x) {
  long m = mid(x);
  return leaf(m + 2);
}
