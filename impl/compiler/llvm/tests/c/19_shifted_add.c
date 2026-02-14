// Shifted add peephole tests:
//   slli tmp, x, k
//   add  dst, base, tmp
// should fold to a single add-with-shamt form when possible.

__attribute__((noinline)) long add_sh3(long a, long b) {
  long t = b << 3;
  return a + t;
}

__attribute__((noinline)) long add_sh7(long a, long b) { return a + (b << 7); }

__attribute__((noinline)) int addw_sh2(int a, int b) {
  int t = b << 2;
  return a + t;
}

__attribute__((noinline)) unsigned int addw_sh1(unsigned int a,
                                                unsigned int b) {
  return a + (b << 1);
}

