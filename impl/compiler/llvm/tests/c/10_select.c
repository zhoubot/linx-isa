long select_i64(long a, long b, long t, long f) { return (a < b) ? t : f; }

unsigned long select_u64(unsigned long a, unsigned long b, unsigned long t,
                         unsigned long f) {
  return (a < b) ? t : f;
}

int select_i32(int a, int b, int t, int f) { return (a < b) ? t : f; }

unsigned select_u32(unsigned a, unsigned b, unsigned t, unsigned f) {
  return (a < b) ? t : f;
}
