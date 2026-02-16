#include <stdint.h>

static __attribute__((noinline)) int64_t local_add1(int64_t x) {
    return x + 1;
}

static __attribute__((noinline)) int64_t local_mix(int64_t x) {
    int64_t y = local_add1(x);
    return y + local_add1(y);
}

int64_t callret_local_reloc(int64_t x) {
    return local_mix(x) + local_add1(x + 3);
}
