// Force aligned constants to be materialized via `lui` (imm20 << 12).

__attribute__((noinline)) int aligned_i32(void) { return 0x12345000; }

__attribute__((noinline)) long long aligned_i64(void) { return 0x12345000LL; }

__attribute__((noinline)) int neg_aligned_i32(void) { return -4096; }

__attribute__((noinline)) long long neg_aligned_i64(void) { return -4096LL; }

