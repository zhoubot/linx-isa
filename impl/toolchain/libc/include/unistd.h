#ifndef _LINX_UNISTD_H
#define _LINX_UNISTD_H

/* Minimal `unistd.h` for freestanding bring-up.
 *
 * This header exists to unblock third-party C benchmarks that include common
 * POSIX headers but do not require full OS services in the bring-up profile.
 *
 * Add declarations here only when needed by workloads.
 */

#include <stddef.h>

#ifndef _LINX_SSIZE_T_DEFINED
#define _LINX_SSIZE_T_DEFINED 1
typedef ptrdiff_t ssize_t;
#endif

#endif /* _LINX_UNISTD_H */
