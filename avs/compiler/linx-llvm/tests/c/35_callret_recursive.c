static __attribute__((noinline)) long sum_to_n(long n) {
  if (n <= 0)
    return 0;
  return n + sum_to_n(n - 1);
}

long callret_recursive(long n) { return sum_to_n(n); }
