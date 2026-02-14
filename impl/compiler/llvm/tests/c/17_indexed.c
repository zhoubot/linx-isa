// Indexed addressing tests: these are intended to exercise the reg-offset
// load/store forms (base + (idx<<shamt)) when the index is not a constant.

signed char load_i8_idx(const signed char *p, int i) { return p[i]; }
void store_i8_idx(signed char *p, int i, signed char v) { p[i] = v; }

int load_i32_idx(const int *p, int i) { return p[i]; }
void store_i32_idx(int *p, int i, int v) { p[i] = v; }

long load_i64_idx(const long *p, int i) { return p[i]; }
void store_i64_idx(long *p, int i, long v) { p[i] = v; }

