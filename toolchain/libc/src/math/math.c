/*
 * linx-libc: minimal libm stubs for bring-up
 *
 * The initial Linx bring-up environment is freestanding (no hosted libc/libm).
 * A small subset of libm is needed to compile and run real-world workloads.
 *
 * This file intentionally implements only lightweight functionality. Where a
 * full implementation is not yet available, functions return conservative
 * placeholders so that code can link and execute under QEMU.
 */

#include <linxisa_libc.h>
#include <math.h>

static inline u64 linx_f64_bits(double x)
{
    union {
        double f;
        u64 u;
    } v;
    v.f = x;
    return v.u;
}

static inline double linx_f64_from_bits(u64 x)
{
    union {
        double f;
        u64 u;
    } v;
    v.u = x;
    return v.f;
}

static inline u32 linx_f32_bits(float x)
{
    union {
        float f;
        u32 u;
    } v;
    v.f = x;
    return v.u;
}

static inline float linx_f32_from_bits(u32 x)
{
    union {
        float f;
        u32 u;
    } v;
    v.u = x;
    return v.f;
}

double fabs(double x)
{
    return linx_f64_from_bits(linx_f64_bits(x) & ~(1ULL << 63));
}

float fabsf(float x)
{
    return linx_f32_from_bits(linx_f32_bits(x) & ~(1U << 31));
}

double sqrt(double x)
{
    if (x <= 0.0) {
        return x == 0.0 ? 0.0 : linx_f64_from_bits(0x7ff8000000000000ULL);
    }

    /* Newton-Raphson with a crude initial guess. */
    double g = x;
    for (int i = 0; i < 16; i++) {
        g = 0.5 * (g + x / g);
    }
    return g;
}

float sqrtf(float x)
{
    return (float)sqrt((double)x);
}

double cos(double x)
{
    (void)x;
    return 0.0;
}

double sin(double x)
{
    (void)x;
    return 0.0;
}

double acos(double x)
{
    (void)x;
    return 0.0;
}

double atan(double x)
{
    /* Minimal special-case used by some codelets: atan(1) == pi/4. */
    if (x == 1.0) {
        return 0.7853981633974483;
    }
    (void)x;
    return 0.0;
}

double pow(double x, double y)
{
    (void)y;
    return x;
}

static inline double linx_f64_pos_inf(void)
{
    return linx_f64_from_bits(0x7ff0000000000000ULL);
}

static inline double linx_f64_neg_inf(void)
{
    return linx_f64_from_bits(0xfff0000000000000ULL);
}

static inline double linx_f64_qnan(void)
{
    return linx_f64_from_bits(0x7ff8000000000000ULL);
}

static inline int linx_f64_is_nan(u64 bits)
{
    return (((bits >> 52) & 0x7ff) == 0x7ff) && ((bits & 0x000fffffffffffffULL) != 0);
}

static inline int linx_f64_is_inf(u64 bits)
{
    return (((bits >> 52) & 0x7ff) == 0x7ff) && ((bits & 0x000fffffffffffffULL) == 0);
}

static double linx_pow2_int(int e)
{
    if (e > 1023) {
        return linx_f64_pos_inf();
    }
    if (e < -1074) {
        return 0.0;
    }
    if (e < -1022) {
        const int shift = e + 1074; /* 0..51 */
        return linx_f64_from_bits((u64)1ULL << (u64)shift);
    }
    return linx_f64_from_bits((u64)(e + 1023) << 52);
}

double exp(double x)
{
    const u64 bits = linx_f64_bits(x);
    if (linx_f64_is_nan(bits)) {
        return x;
    }
    if (linx_f64_is_inf(bits)) {
        if (bits >> 63) {
            return 0.0;
        }
        return linx_f64_pos_inf();
    }

    /* Clamp to avoid overflow/underflow. */
    if (x > 709.782712893384) {
        return linx_f64_pos_inf();
    }
    if (x < -745.1332191019411) {
        return 0.0;
    }

    /* Range-reduce using x = n*ln2 + r, r in ~[-ln2/2, ln2/2]. */
    const double ln2 = 0.6931471805599453;
    const double invln2 = 1.4426950408889634;

    int n = (int)(x * invln2 + (x >= 0.0 ? 0.5 : -0.5));
    double r = x - (double)n * ln2;

    /* exp(r) via a short Taylor series around 0. */
    double term = 1.0;
    double sum = 1.0;
    for (int i = 1; i <= 12; i++) {
        term *= r / (double)i;
        sum += term;
    }

    return sum * linx_pow2_int(n);
}

float expf(float x)
{
    return (float)exp((double)x);
}

double log(double x)
{
    const u64 bits = linx_f64_bits(x);
    if (linx_f64_is_nan(bits)) {
        return x;
    }
    if (linx_f64_is_inf(bits)) {
        if (bits >> 63) {
            return linx_f64_qnan();
        }
        return linx_f64_pos_inf();
    }
    if (x == 0.0) {
        return linx_f64_neg_inf();
    }
    if (x < 0.0) {
        return linx_f64_qnan();
    }

    /* Decompose x = m * 2^e with m in [1,2). */
    u64 exp_bits = (bits >> 52) & 0x7ff;
    u64 mant = bits & 0x000fffffffffffffULL;
    int e = 0;
    if (exp_bits == 0) {
        /* Subnormal: treat as underflow for bring-up. */
        return linx_f64_neg_inf();
    } else {
        e = (int)exp_bits - 1023;
    }

    const double m = 1.0 + (double)mant / (double)(1ULL << 52);

    /* log(m) using atanh-series: log(m) = 2*(y + y^3/3 + y^5/5 + ...)
     * where y = (m-1)/(m+1), and for m in [1,2), y in [0, 1/3]. */
    const double y = (m - 1.0) / (m + 1.0);
    const double y2 = y * y;
    double term = y;
    double acc = term;
    for (int k = 3; k <= 11; k += 2) {
        term *= y2;
        acc += term / (double)k;
    }
    const double ln_m = 2.0 * acc;

    const double ln2 = 0.6931471805599453;
    return ln_m + (double)e * ln2;
}

float logf(float x)
{
    return (float)log((double)x);
}
