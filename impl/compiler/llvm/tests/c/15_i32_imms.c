// Force *IW immediate forms for i32 bitwise ops.
// Use optnone + a stack roundtrip to keep the operation in i32 form.

__attribute__((noinline, optnone)) int andiw_force(int x) {
  volatile int y = x;
  return y & 2047;
}

__attribute__((noinline, optnone)) int oriw_force(int x) {
  volatile int y = x;
  return y | 341;
}

__attribute__((noinline, optnone)) int xoriw_force(int x) {
  volatile int y = x;
  return y ^ 682;
}

