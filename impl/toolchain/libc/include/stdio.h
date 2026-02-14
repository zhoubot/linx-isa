#ifndef _LINX_STDIO_H
#define _LINX_STDIO_H

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Minimal FILE model for freestanding bring-up.
 *
 * Linx libc currently routes all formatted output to the UART console and
 * treats stdout/stderr equivalently. This is enough to compile and run common
 * benchmarks (e.g. PolyBench/C) without pulling in a full hosted stdio stack.
 */
typedef struct linx_FILE FILE;
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

int putchar(int c);
int puts(const char *s);
int printf(const char *format, ...);
int vprintf(const char *format, va_list ap);
int fprintf(FILE *stream, const char *format, ...);
int vfprintf(FILE *stream, const char *format, va_list ap);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
int snprintf(char *str, size_t size, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* _LINX_STDIO_H */
