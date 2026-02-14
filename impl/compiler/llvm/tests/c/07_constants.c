unsigned long const_u32(void) { return 0x89ABCDEFu; }

unsigned long const_u32_allones(void) { return 0xFFFFFFFFu; }

unsigned long and_mask(unsigned long x) { return x & 0x12345678u; }

unsigned long add_mask(unsigned long x) { return (x & 0x12345678u) + 100000u; }
