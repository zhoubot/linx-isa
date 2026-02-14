// Compressed instruction forms (C.* instructions)

// Compressed arithmetic
int c_add(int a, int b) {
  return a + b;
}

int c_sub(int a, int b) {
  return a - b;
}

int c_addi(int a) {
  return a + 5; // Small immediate
}

// Compressed moves
int c_mov(int a) {
  return a;
}

int c_movi(int x) {
  return x + 10; // Small immediate move
}

// Compressed loads/stores
int c_load(int *ptr) {
  return *ptr;
}

void c_store(int *ptr, int val) {
  *ptr = val;
}

// Compressed comparisons
int c_cmp_eq(int a, int b) {
  return (a == b) ? 1 : 0;
}

int c_cmp_ne(int a, int b) {
  return (a != b) ? 1 : 0;
}

// Compressed branches
void c_branch_test(int x) {
  if (x == 0) {
    x = 1;
  }
  if (x != 0) {
    x = 2;
  }
}

// Compressed set condition
int c_setc(int a, int b) {
  return (a < b) ? 1 : 0;
}

// Compressed sign/zero extend
int c_sext_b(char x) {
  return (int)(signed char)x;
}

int c_sext_h(short x) {
  return (int)(signed short)x;
}

int c_zext_b(unsigned char x) {
  return (int)x;
}

int c_zext_h(unsigned short x) {
  return (int)x;
}
