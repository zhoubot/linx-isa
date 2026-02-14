int load_s8(const signed char *p) { return *p; }

unsigned load_u8(const unsigned char *p) { return *p; }

int load_s16(const short *p) { return *p; }

unsigned load_u16(const unsigned short *p) { return *p; }

int load_i32(const int *p) { return *p; }

unsigned long load_u32_zext(const unsigned *p) { return *p; }

unsigned long load_i64(const unsigned long *p) { return *p; }

void store_i8(unsigned char *p, unsigned char v) { *p = v; }

void store_i16(unsigned short *p, unsigned short v) { *p = v; }

void store_i32(unsigned *p, unsigned v) { *p = v; }

void store_i64(unsigned long *p, unsigned long v) { *p = v; }
