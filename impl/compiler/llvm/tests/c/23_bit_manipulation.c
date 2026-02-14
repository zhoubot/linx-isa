// Bit manipulation operations

unsigned count_leading_zeros(unsigned x) {
  if (x == 0) return 32;
  unsigned count = 0;
  while (!(x & 0x80000000)) {
    count++;
    x <<= 1;
  }
  return count;
}

unsigned count_trailing_zeros(unsigned x) {
  if (x == 0) return 32;
  unsigned count = 0;
  while (!(x & 1)) {
    count++;
    x >>= 1;
  }
  return count;
}

unsigned count_ones(unsigned x) {
  unsigned count = 0;
  while (x) {
    count += (x & 1);
    x >>= 1;
  }
  return count;
}

unsigned reverse_bits(unsigned x) {
  unsigned result = 0;
  for (int i = 0; i < 32; i++) {
    result <<= 1;
    result |= (x & 1);
    x >>= 1;
  }
  return result;
}

unsigned extract_bits(unsigned x, unsigned start, unsigned len) {
  return (x >> start) & ((1u << len) - 1);
}

unsigned insert_bits(unsigned x, unsigned val, unsigned start, unsigned len) {
  unsigned mask = ((1u << len) - 1) << start;
  return (x & ~mask) | ((val << start) & mask);
}

unsigned rotate_left(unsigned x, unsigned n) {
  n &= 31;
  return (x << n) | (x >> (32 - n));
}

unsigned rotate_right(unsigned x, unsigned n) {
  n &= 31;
  return (x >> n) | (x << (32 - n));
}

// Bit field operations
struct bitfield {
  unsigned a : 5;
  unsigned b : 3;
  unsigned c : 8;
  unsigned d : 16;
};

unsigned get_bitfield_a(struct bitfield *bf) { return bf->a; }
void set_bitfield_a(struct bitfield *bf, unsigned val) { bf->a = val; }

unsigned get_bitfield_b(struct bitfield *bf) { return bf->b; }
void set_bitfield_b(struct bitfield *bf, unsigned val) { bf->b = val; }
