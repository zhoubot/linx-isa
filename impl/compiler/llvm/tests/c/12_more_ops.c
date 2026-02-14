long ashr_i64(long x, unsigned sh) { return x >> (sh & 63u); }

long ashr_i64_imm(long x) { return x >> 7; }

int ashr_i32(int x, unsigned sh) { return x >> (sh & 31u); }

unsigned lshr_u32_imm(unsigned x) { return x >> 3; }

unsigned lshr_u32(unsigned x, unsigned sh) { return x >> (sh & 31u); }

int shl_i32(int x, unsigned sh) { return x << (sh & 31u); }

// Signed division/remainder (forces non-zero divisor).
long div_i64(long a, long b) {
  b |= 1;
  return a / b;
}

long mod_i64(long a, long b) {
  b |= 1;
  return a % b;
}

int div_i32(int a, int b) {
  b |= 1;
  return a / b;
}

int mod_i32(int a, int b) {
  b |= 1;
  return a % b;
}

unsigned mod_u32(unsigned a, unsigned b) {
  b |= 1u;
  return a % b;
}

unsigned long mod_u64(unsigned long a, unsigned long b) {
  b |= 1ul;
  return a % b;
}

// Immediate ops (exercise *I/*IW variants).
int andiw_i32(int x) { return x & 0x7ff; }

int oriw_i32(int x) { return x | 0x155; }

int xoriw_i32(int x) { return x ^ 0x2aa; }

unsigned long xori_i64(unsigned long x) { return x ^ 0x5a5; }

// Force CMP.NE / CMP.GE / CMP.GEU (0/1 result), not just branches.
int cmp_ne_i32(int a, int b) { return a != b; }

int cmp_ge_i32(int a, int b) { return a >= b; }

unsigned cmp_geu_u32(unsigned a, unsigned b) { return a >= b; }

// Force B.GE / B.GEU in control-flow.
int count_down_ge(int n) {
  int acc = 0;
  for (int i = n; i >= 0; --i)
    acc += i;
  return acc;
}

unsigned count_down_geu(unsigned n) {
  unsigned acc = 0;
  while (n >= 1u) {
    acc += n;
    --n;
  }
  return acc;
}
