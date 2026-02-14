/*
 * linx-libc: LinxISA-specific system call stubs
 * 
 * These functions provide the interface between the C library
 * and the underlying LinxISA system.
 */

#include <linxisa_libc.h>

/* File descriptors */
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/*
 * __linx_putchar - Write a character to stdout
 * 
 * This is the core output function. On real hardware/emulator,
 * this would make a syscall to write to a console or UART.
 */
void __linx_putchar(int c) {
    volatile unsigned char *uart = (volatile unsigned char *)0x10000000;
    *uart = (unsigned char)c;
}

/*
 * __linx_puts - Write a null-terminated string to stdout
 */
void __linx_puts(const char *s) {
    while (*s) {
        __linx_putchar(*s++);
    }
    __linx_putchar('\n');
}

/*
 * __linx_exit - Terminate the program
 * 
 * This should never return - the program is terminated.
 */
void __linx_exit(int code) {
    /* Exit register is at UART base + 0x4 (virt machine). */
    volatile unsigned int *mmio = (volatile unsigned int *)0x10000000;
    mmio[1] = (unsigned int)code;
    
    /* If exit doesn't halt, loop forever */
    while (1) {
        __asm__ volatile ("" ::: "memory");
    }
}

/*
 * __linx_read - Read from a file descriptor
 * 
 * Returns the number of bytes read, or -1 on error.
 */
int __linx_read(int fd, void *buf, size_t count) {
    /* TODO: Implement actual read syscall */
    (void)fd;
    (void)buf;
    (void)count;
    return -1;
}

/*
 * __linx_write - Write to a file descriptor
 * 
 * Returns the number of bytes written, or -1 on error.
 */
int __linx_write(int fd, const void *buf, size_t count) {
    if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
        const char *p = (const char *)buf;
        size_t written = 0;
        while (written < count) {
            __linx_putchar(p[written++]);
        }
        if (written > 0x7fffffffU) {
            return 0x7fffffff;
        }
        return (int)written;
    }
    /* TODO: Implement write for other file descriptors */
    return -1;
}

/*
 * __linx_open - Open a file
 * 
 * Returns a file descriptor, or -1 on error.
 */
int __linx_open(const char *pathname, int flags, int mode) {
    (void)pathname;
    (void)flags;
    (void)mode;
    return -1;
}

/*
 * __linx_close - Close a file descriptor
 */
int __linx_close(int fd) {
    (void)fd;
    return 0;
}

/*
 * __linx_brk - Change data segment size
 */
void *__linx_brk(void *addr) {
    (void)addr;
    return NULL;
}
