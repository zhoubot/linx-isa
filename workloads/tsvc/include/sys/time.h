#ifndef LINX_TSVC_SYS_TIME_H
#define LINX_TSVC_SYS_TIME_H

#include <time.h>

struct timeval {
  long tv_sec;
  long tv_usec;
};

int gettimeofday(struct timeval *__restrict__, void *__restrict__);

#endif
