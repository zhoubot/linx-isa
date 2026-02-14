long sum_upto(long n) {
  long sum = 0;
  // Keep the loop form (avoid closed-form strength reduction).
  for (long i = 0; i < n; ++i)
    sum += (i ^ (n >> 1));
  return sum;
}

int classify_i32(int x) {
  if (x < 0)
    return -1;
  if (x == 0)
    return 0;
  return 1;
}

unsigned long popcount_u64(unsigned long x) {
  unsigned long c = 0;
  while (x) {
    c += (x & 1u);
    x >>= 1;
  }
  return c;
}

int loop_mix(int n) {
  int acc = 0;
  for (int i = 0; i < n; ++i) {
    if ((i & 1) == 0)
      acc += i;
    else
      acc -= i;
  }
  return acc;
}
