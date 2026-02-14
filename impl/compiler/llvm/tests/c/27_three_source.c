// Three-source operations (fused multiply-add, etc.)

// Fused multiply-add
long fma_i64(long a, long b, long c) {
  return a * b + c;
}

int fma_i32(int a, int b, int c) {
  return a * b + c;
}

// Multiply-subtract
long fms_i64(long a, long b, long c) {
  return a * b - c;
}

int fms_i32(int a, int b, int c) {
  return a * b - c;
}

// Three-source bitwise operations
unsigned three_source_and(unsigned a, unsigned b, unsigned c) {
  return (a & b) | c;
}

unsigned three_source_or(unsigned a, unsigned b, unsigned c) {
  return (a | b) & c;
}

// Complex arithmetic patterns
long complex_three_source(long a, long b, long c) {
  return (a + b) * c;
}

int complex_three_source_i32(int a, int b, int c) {
  return (a - b) * c;
}

// Conditional three-source
long conditional_fma(long a, long b, long c, int cond) {
  if (cond) {
    return a * b + c;
  } else {
    return a * b - c;
  }
}
