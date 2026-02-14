#ifndef _LINX_STDLIB_H
#define _LINX_STDLIB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void exit(int status);
void abort(void);

int atexit(void (*func)(void));

void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* _LINX_STDLIB_H */
