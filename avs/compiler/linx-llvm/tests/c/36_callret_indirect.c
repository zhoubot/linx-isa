typedef long (*binop_fn)(long, long);

static __attribute__((noinline)) long add_impl(long a, long b) { return a + b; }
static __attribute__((noinline)) long mul_impl(long a, long b) { return a * b; }

static __attribute__((noinline)) long dispatch(binop_fn fn, long a, long b) {
  return fn(a, b);
}

long callret_indirect(long x) {
  long a = dispatch(add_impl, x, 4);
  long b = dispatch(mul_impl, a, 3);
  return b - 5;
}
