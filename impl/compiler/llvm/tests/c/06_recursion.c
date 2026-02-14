static int fib_impl(int n) {
  if (n <= 1)
    return n;
  return fib_impl(n - 1) + fib_impl(n - 2);
}

int fib(int n) { return fib_impl(n); }

static unsigned long fact_impl(unsigned long n) {
  if (n <= 1)
    return 1;
  return n * fact_impl(n - 1);
}

unsigned long fact(unsigned long n) { return fact_impl(n); }
