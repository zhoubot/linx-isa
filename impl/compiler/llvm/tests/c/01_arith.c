int add_i32(int a, int b) { return a + b; }

int sub_i32(int a, int b) { return a - b; }

unsigned mul_u32(unsigned a, unsigned b) { return a * b; }

unsigned divrem_u32(unsigned a, unsigned b) {
  b |= 1u;
  return (a / b) + (a % b);
}

long add_i64(long a, long b) { return a + b - 123456L; }

unsigned long mul_u64(unsigned long a, unsigned long b) { return a * b; }

unsigned long divrem_u64(unsigned long a, unsigned long b) {
  b |= 1ul;
  return (a / b) + (a % b);
}
