/* linx-libc: Minimal C library for LinxISA */

/* Version 0.1 */

#ifndef _LINX_LIBC_H
#define _LINX_LIBC_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

/* Compiler helpers */
#define __LINX_WEAK __attribute__((weak))
#define __LINX_NORETURN __attribute__((noreturn))

/* Basic types */
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/* Size_t and related */
#ifndef NULL
#define NULL ((void*)0)
#endif

/* POSIX-like signed size for printf's %zd, etc. */
typedef ptrdiff_t ssize_t;

/* Architecture-specific macros */
#define __LINX_ISA__ linx64

/* System calls (implemented in architecture-specific code) */
void __linx_putchar(int c);
void __linx_puts(const char *s);
void __linx_exit(int code) __LINX_NORETURN;
int __linx_read(int fd, void *buf, size_t count);
int __linx_write(int fd, const void *buf, size_t count);

/* Memory functions */
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void *memmove(void *dest, const void *src, size_t n);

/* String functions */
size_t strlen(const char *s);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
char *strstr(const char *haystack, const char *needle);

/* Standard I/O */
int putchar(int c);
int puts(const char *s);
int printf(const char *format, ...);
int vprintf(const char *format, va_list ap);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
int snprintf(char *str, size_t size, const char *format, ...);

/* Standard library */
void exit(int status) __LINX_WEAK;
void *malloc(size_t size) __LINX_WEAK;
void free(void *ptr) __LINX_WEAK;
void *realloc(void *ptr, size_t size) __LINX_WEAK;
void abort(void) __LINX_WEAK;

/* atexit handling */
int atexit(void (*func)(void));

/* Dynamic memory allocation hooks */
void *__linx_alloc(size_t size);
void __linx_free(void *ptr);

#endif /* _LINX_LIBC_H */
