/* linx-libc: Architecture-specific system calls */

#ifndef _LINX_SYSCALL_H
#define _LINX_SYSCALL_H

/* LinxISA system call numbers */
#define SYS_exit    1
#define SYS_read    2
#define SYS_write   3
#define SYS_open    4
#define SYS_close   5
#define SYS_brk      6
#define SYS_lseek   7
#define SYS_mmap    8
#define SYS_munmap  9
#define SYS_getpid  10
#define SYS_fork    11
#define SYS_execve  12
#define SYS_wait    13
#define SYS_ioctl   14

/* LinxISA specific syscalls */
#define SYS_putchar  100
#define SYS_puts     101
#define SYS_debug    102

/* File descriptors */
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#endif /* _LINX_SYSCALL_H */
