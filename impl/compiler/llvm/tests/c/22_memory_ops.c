// Extended memory operations: load/store pairs, pre/post-index, unscaled

// Load/store pairs
void load_pair_i64(long *dst, long *src) {
  long a = src[0];
  long b = src[1];
  dst[0] = a;
  dst[1] = b;
}

void load_pair_i32(int *dst, int *src) {
  int a = src[0];
  int b = src[1];
  dst[0] = a;
  dst[1] = b;
}

// Pre-indexed loads/stores
long load_pre_index(long *base, long offset) {
  long *addr = base + offset;
  return *addr;
}

void store_pre_index(long *base, long offset, long val) {
  long *addr = base + offset;
  *addr = val;
}

// Post-indexed loads/stores
long load_post_index(long *base, long offset) {
  long val = *base;
  base += offset;
  return val;
}

void store_post_index(long *base, long offset, long val) {
  *base = val;
  base += offset;
}

// Unscaled loads/stores (small offsets)
int load_unscaled(int *base, int offset) {
  char *addr = (char *)base + offset;
  return *(int *)addr;
}

void store_unscaled(int *base, int offset, int val) {
  char *addr = (char *)base + offset;
  *(int *)addr = val;
}

// PC-relative loads
long load_pc_relative(void) {
  extern long pc_rel_var;
  return pc_rel_var;
}

void store_pc_relative(long val) {
  extern long pc_rel_var;
  pc_rel_var = val;
}

// Register-offset loads/stores
long load_reg_offset(long *base, long offset) {
  return base[offset];
}

void store_reg_offset(long *base, long offset, long val) {
  base[offset] = val;
}

// Scaled indexed loads/stores
int load_scaled_index(int *base, int index, int scale) {
  return base[index * scale];
}

void store_scaled_index(int *base, int index, int scale, int val) {
  base[index * scale] = val;
}
