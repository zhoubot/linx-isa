#include <math.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>

int gettimeofday(struct timeval *tv, void *tz) {
    static uint64_t fake_us = 0;
    (void)tz;
    if (!tv) {
        return -1;
    }
    fake_us += 1000;
    tv->tv_sec = (long)(fake_us / 1000000ull);
    tv->tv_usec = (long)(fake_us % 1000000ull);
    return 0;
}

void *memalign(size_t alignment, size_t size) {
    (void)alignment;
    return malloc(size);
}

float sinf(float x) {
    return (float)sin((double)x);
}

float cosf(float x) {
    return (float)cos((double)x);
}
