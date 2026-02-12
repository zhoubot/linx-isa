#ifndef LINX_COMPAT_STDLIB_H
#define LINX_COMPAT_STDLIB_H

#include_next <stdlib.h>

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

#endif
