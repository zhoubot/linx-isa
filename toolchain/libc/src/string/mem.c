/*
 * linx-libc: Memory functions
 * These are optimized implementations for LinxISA
 */

#include <linxisa_libc.h>

void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    /*
     * Keep the bring-up memcpy simple and correct:
     * - Avoid unsigned underflow loop idioms (n--) that can be fragile across
     *   early toolchain bring-up changes.
     * - Use end-pointer comparisons, which compile well on LinxISA.
     */
    unsigned char *end = d + n;

    /* Byte-copy until 4-byte aligned (if possible). */
    while (((uintptr_t)d & 3) && d != end) {
        *d++ = *s++;
    }

    /* Bulk copy 4 bytes at a time when both pointers are aligned and enough remains. */
    while ((uintptr_t)s % 4 == 0 && (uintptr_t)d % 4 == 0 && (size_t)(end - d) >= 4) {
        *(uint32_t *)(void *)d = *(const uint32_t *)(const void *)s;
        d += 4;
        s += 4;
    }

    /* Tail bytes. */
    while (d != end) {
        *d++ = *s++;
    }

    return dest;
}

void *memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    unsigned char uc = (unsigned char)c;
    
    /* Set all bytes to c */
    while (n--) {
        *p++ = uc;
    }
    return s;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *a = (const unsigned char *)s1;
    const unsigned char *b = (const unsigned char *)s2;
    while (n--) {
        if (*a != *b) {
            return *a - *b;
        }
        a++;
        b++;
    }
    return 0;
}

void *memmove(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    
    if (d <= s) {
        /* Forward copy */
        while (n--) {
            *d++ = *s++;
        }
    } else {
        /* Backward copy */
        d += n;
        s += n;
        while (n--) {
            *--d = *--s;
        }
    }
    
    return dest;
}
