#ifndef TSVC_ARRAY_HDR
#define TSVC_ARRAY_HDR

// Arrays used by TSVC and common.c

#define ARRAY_ALIGNMENT 64

extern __attribute__((aligned(ARRAY_ALIGNMENT))) real_t flat_2d_array[LEN_2D*LEN_2D];

extern __attribute__((aligned(ARRAY_ALIGNMENT))) real_t x[LEN_1D];

extern __attribute__((aligned(ARRAY_ALIGNMENT))) real_t a[LEN_1D],b[LEN_1D],c[LEN_1D],d[LEN_1D],e[LEN_1D],
                                   aa[LEN_2D][LEN_2D],bb[LEN_2D][LEN_2D],cc[LEN_2D][LEN_2D],tt[LEN_2D][LEN_2D];

extern __attribute__((aligned(ARRAY_ALIGNMENT))) int indx[LEN_1D];

extern real_t* __restrict__ xx;
extern real_t* yy;

#endif
