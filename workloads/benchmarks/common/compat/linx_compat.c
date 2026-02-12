#include <malloc.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

void *memalign(size_t alignment, size_t size) {
    if (alignment < sizeof(void *)) {
        alignment = sizeof(void *);
    }
    if ((alignment & (alignment - 1)) != 0) {
        return NULL;
    }

    size_t extra = alignment - 1 + sizeof(uintptr_t);
    void *raw = malloc(size + extra);
    if (!raw) {
        return NULL;
    }

    uintptr_t base = (uintptr_t)raw + sizeof(uintptr_t);
    uintptr_t aligned = (base + (alignment - 1)) & ~(uintptr_t)(alignment - 1);
    ((uintptr_t *)aligned)[-1] = (uintptr_t)raw;
    return (void *)aligned;
}

int gettimeofday(struct timeval *tv, struct timezone *tz) {
    static unsigned long long usec = 0;
    usec += 1000;
    if (tv) {
        tv->tv_sec = (long)(usec / 1000000ULL);
        tv->tv_usec = (long)(usec % 1000000ULL);
    }
    if (tz) {
        tz->tz_minuteswest = 0;
        tz->tz_dsttime = 0;
    }
    return 0;
}

clock_t clock(void) {
    static clock_t ticks = 0;
    ticks += 1000;
    return ticks;
}

float sinf(float x) {
    return (float)sin((double)x);
}

float cosf(float x) {
    return (float)cos((double)x);
}
