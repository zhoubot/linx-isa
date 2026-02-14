/*
 * linx-libc: Exit and stdlib functions
 */

#include <linxisa_libc.h>

/* atexit handlers */
#define ATEXIT_MAX 32
static void (*atexit_funcs[ATEXIT_MAX])(void);
static int atexit_count = 0;

/* Minimal bump allocator for freestanding bring-up.
 *
 * - No free list; free() is a no-op.
 * - realloc() allocates a new block and copies.
 * - The heap lives in .bss so it works for ET_REL images loaded by QEMU.
 *
 * This is intentionally simple; it exists to unblock real-world C workloads
 * that expect malloc to work (e.g. ctuning Milepost codelets).
 */
#ifndef LINX_HEAP_SIZE
#define LINX_HEAP_SIZE (16u * 1024u * 1024u)
#endif

static unsigned char linx_heap[LINX_HEAP_SIZE];
static size_t linx_heap_off;

static size_t linx_align_up(size_t v, size_t align)
{
    if (align <= 1) {
        return v;
    }
    return (v + align - 1) & ~(align - 1);
}

static size_t linx_min_size(size_t a, size_t b)
{
    return (a < b) ? a : b;
}

int atexit(void (*func)(void)) {
    if (atexit_count >= ATEXIT_MAX) {
        return -1;
    }
    atexit_funcs[atexit_count++] = func;
    return 0;
}

void __linx_do_exit(int code) {
    /* Call atexit handlers in reverse order */
    while (atexit_count > 0) {
        atexit_funcs[--atexit_count]();
    }
    
    /* Call system exit */
    __linx_exit(code);
}

/* Default exit implementation (can be overridden) */
void exit(int code) __attribute__((weak));
void exit(int code) {
    __linx_do_exit(code);
}

/* Default abort implementation */
void abort(void) __attribute__((weak));
void abort(void) {
    __linx_exit(1);
}

/* Default malloc (freestanding bump allocator) */
void *malloc(size_t size) __attribute__((weak));
void *malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    const size_t align = (size_t)_Alignof(max_align_t);
    const size_t user = linx_align_up(linx_heap_off + sizeof(size_t), align);
    const size_t header = user - sizeof(size_t);

    if (user > LINX_HEAP_SIZE || size > LINX_HEAP_SIZE - user) {
        return NULL;
    }

    *((size_t *)(void *)(linx_heap + header)) = size;
    linx_heap_off = user + size;
    return (void *)(linx_heap + user);
}

void free(void *ptr) __attribute__((weak));
void free(void *ptr) {
    (void)ptr;
}

void *realloc(void *ptr, size_t size) __attribute__((weak));
void *realloc(void *ptr, size_t size) {
    if (!ptr) {
        return malloc(size);
    }
    if (size == 0) {
        return NULL;
    }

    size_t *header = ((size_t *)ptr) - 1;
    const size_t old_size = *header;
    void *new_ptr = malloc(size);
    if (!new_ptr) {
        return NULL;
    }

    memcpy(new_ptr, ptr, linx_min_size(old_size, size));
    return new_ptr;
}

/* Dynamic allocation hooks */
void *__linx_alloc(size_t size) {
    return malloc(size);
}

void __linx_free(void *ptr) {
    free(ptr);
}

/* Quick exit for when we can't return from main */
void _exit(int code) {
    __linx_do_exit(code);
}
