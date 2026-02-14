unsigned long pack_bytes(unsigned char a, unsigned char b, unsigned char c,
                         unsigned char d) {
  unsigned long x = 0;
  x |= (unsigned long)a;
  x |= (unsigned long)b << 8;
  x |= (unsigned long)c << 16;
  x |= (unsigned long)d << 24;
  return x;
}

unsigned long rotl_u64(unsigned long x, unsigned sh) {
  sh &= 63u;
  return (x << sh) | (x >> ((64u - sh) & 63u));
}

unsigned long rotr_u64(unsigned long x, unsigned sh) {
  sh &= 63u;
  return (x >> sh) | (x << ((64u - sh) & 63u));
}
