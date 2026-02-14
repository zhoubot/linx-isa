// Immediate materialization - testing LUI, ADDI, HL.LUI, HL.ADDI patterns

// Small immediates (should use ADDI)
int small_immediate_add(int x) {
  return x + 42;
}

int small_immediate_sub(int x) {
  return x - 42;
}

// Medium immediates (may need LUI + ADDI)
int medium_immediate(int x) {
  return x + 0x1234;
}

// Large immediates (should use HL.LUI + HL.ADDI or similar)
long large_immediate_32(long x) {
  return x + 0x12345678L;
}

long large_immediate_64(long x) {
  return x + 0x123456789ABCDEF0L;
}

// Immediate in comparisons
int compare_immediate(int x) {
  if (x == 0x1234)
    return 1;
  if (x < 0x5678)
    return 2;
  return 0;
}

// Immediate in bitwise operations
unsigned bitwise_immediate(unsigned x) {
  return (x & 0xFF00FF00) | (x | 0x00FF00FF);
}

// Immediate shifts
unsigned shift_immediate(unsigned x) {
  return (x << 5) | (x >> 27);
}

// Multiple immediates in one function
int multiple_immediates(int x) {
  int a = x + 10;
  int b = a - 20;
  int c = b + 0x1000;
  return c - 0x2000;
}

// Immediate materialization for addresses
void *address_immediate(void) {
  extern char data_section[];
  return (void *)data_section;
}

// PC-relative immediate (for labels)
long pc_relative_immediate(void) {
  extern long pc_rel_symbol;
  return pc_rel_symbol;
}
