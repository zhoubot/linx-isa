/*
 * linx-libc: Soft-fp (software floating point) library
 * 
 * This provides software implementations of floating point operations
 * for targets without hardware FPU.
 */

#include <linxisa_libc.h>

/* Float to bits and back */
typedef u32 f32_bits;
typedef u64 f64_bits;

#define F32_SIGN_SHIFT   31
#define F32_EXP_SHIFT    23
#define F32_EXP_MASK     0xFF
#define F32_MANT_MASK    0x007FFFFF
#define F32_IMPLICIT     0x00800000
#define F32_SIGN_BIT     (1U << 31)

#define F64_SIGN_SHIFT   63
#define F64_EXP_SHIFT    52
#define F64_EXP_MASK     0x7FF
#define F64_MANT_MASK    0x000FFFFFFFFFFFFF
#define F64_IMPLICIT     0x0010000000000000
#define F64_SIGN_BIT     (1ULL << 63)

/* Get exponent bias */
#define F32_BIAS  127
#define F64_BIAS 1023

/* Check for special values */
static inline int f32_is_inf(f32_bits x) {
    return ((x >> F32_EXP_SHIFT) & F32_EXP_MASK) == F32_EXP_MASK 
           && ((x & F32_MANT_MASK) == 0);
}

static inline int f32_is_nan(f32_bits x) {
    return ((x >> F32_EXP_SHIFT) & F32_EXP_MASK) == F32_EXP_MASK 
           && ((x & F32_MANT_MASK) != 0);
}

static inline int f32_is_zero(f32_bits x) {
    return (x & ~F32_SIGN_BIT) == 0;
}

static inline int f64_is_inf(f64_bits x) {
    return ((x >> F64_EXP_SHIFT) & F64_EXP_MASK) == F64_EXP_MASK 
           && ((x & F64_MANT_MASK) == 0);
}

static inline int f64_is_nan(f64_bits x) {
    return ((x >> F64_EXP_SHIFT) & F64_EXP_MASK) == F64_EXP_MASK 
           && ((x & F64_MANT_MASK) != 0);
}

static inline int f64_is_zero(f64_bits x) {
    return (x & ~F64_SIGN_BIT) == 0;
}

static inline f64_bits f64_to_bits(double x)
{
    union {
        double f;
        f64_bits u;
    } v;
    v.f = x;
    return v.u;
}

static inline double f64_from_bits(f64_bits x)
{
    union {
        double f;
        f64_bits u;
    } v;
    v.u = x;
    return v.f;
}

static inline f32_bits f32_to_bits(float x)
{
    union {
        float f;
        f32_bits u;
    } v;
    v.f = x;
    return v.u;
}

static inline float f32_from_bits(f32_bits x)
{
    union {
        float f;
        f32_bits u;
    } v;
    v.u = x;
    return v.f;
}

static inline float f32_qnan(void) { return f32_from_bits(0x7FC00000u); }
static inline float f32_inf(int sign) {
    return f32_from_bits(((u32)sign << F32_SIGN_SHIFT) | (F32_EXP_MASK << F32_EXP_SHIFT));
}

/* Float addition */
float __addsf3(float a, float b) {
    const f32_bits A = f32_to_bits(a);
    const f32_bits B = f32_to_bits(b);

    if (f32_is_nan(A)) return a;
    if (f32_is_nan(B)) return b;

    int signA = (int)((A >> F32_SIGN_SHIFT) & 1);
    int signB = (int)((B >> F32_SIGN_SHIFT) & 1);
    int expA = (int)((A >> F32_EXP_SHIFT) & F32_EXP_MASK);
    int expB = (int)((B >> F32_EXP_SHIFT) & F32_EXP_MASK);
    u32 mantA = (u32)(A & F32_MANT_MASK);
    u32 mantB = (u32)(B & F32_MANT_MASK);

    /* Inf/NaN handling */
    if (expA == F32_EXP_MASK) {
        if (mantA) return a; /* NaN */
        if (expB == F32_EXP_MASK && mantB == 0 && signA != signB)
            return f32_qnan(); /* +inf + -inf = NaN */
        return a;
    }
    if (expB == F32_EXP_MASK) {
        if (mantB) return b; /* NaN */
        return b;
    }

    /* Zeros */
    if (f32_is_zero(A)) return b;
    if (f32_is_zero(B)) return a;

    /* Treat subnormals as having exponent 1 for alignment. */
    int eA = expA ? expA : 1;
    int eB = expB ? expB : 1;
    if (expA) mantA |= F32_IMPLICIT;
    if (expB) mantB |= F32_IMPLICIT;

    /* Ensure A has the larger exponent. */
    if (eB > eA) {
        int t;
        u32 mt;
        t = eA; eA = eB; eB = t;
        t = expA; expA = expB; expB = t;
        t = signA; signA = signB; signB = t;
        mt = mantA; mantA = mantB; mantB = mt;
    }

    /* Use extended mantissas with 3 LSBs for guard/round/sticky. */
    u32 mantA_ext = mantA << 3;
    u32 mantB_ext = mantB << 3;
    int expRes = eA;

    int sRes = signA;
    int sBv = signB;

    const int diff = eA - eB;
    if (diff) {
        if (diff >= 31) {
            mantB_ext = 1; /* sticky */
        } else {
            const u32 mask = (1u << diff) - 1u;
            const u32 sticky = (mantB_ext & mask) ? 1u : 0u;
            mantB_ext >>= diff;
            mantB_ext |= sticky;
        }
    }

    u32 mantRes;
    int signRes;
    if (sRes == sBv) {
        mantRes = mantA_ext + mantB_ext;
        signRes = sRes;
        if (mantRes & (1u << (24 + 3))) {
            /* carry: renormalize right by 1 */
            const u32 sticky = (mantRes & 1u);
            mantRes >>= 1;
            mantRes |= sticky;
            expRes++;
        }
    } else {
        if (mantA_ext >= mantB_ext) {
            mantRes = mantA_ext - mantB_ext;
            signRes = sRes;
        } else {
            mantRes = mantB_ext - mantA_ext;
            signRes = sBv;
        }
        if (mantRes == 0) {
            return f32_from_bits(0);
        }
        while ((mantRes & (1u << (23 + 3))) == 0) {
            mantRes <<= 1;
            expRes--;
            if (expRes <= 0) {
                break;
            }
        }
    }

    /* Round-to-nearest-even from (24+3) bits down to 24. */
    u32 mantMain = mantRes >> 3;
    const u32 guard = (mantRes >> 2) & 1u;
    const u32 roundb = (mantRes >> 1) & 1u;
    const u32 sticky = mantRes & 1u;
    if (guard && (roundb || sticky || (mantMain & 1u))) {
        mantMain++;
        if (mantMain == (1u << 24)) {
            mantMain >>= 1;
            expRes++;
        }
    }

    if (expRes >= F32_EXP_MASK) {
        return f32_inf(signRes);
    }

    if (expRes <= 0) {
        /* Underflow/subnormal (flush to zero for bring-up). */
        return f32_from_bits((u32)signRes << F32_SIGN_SHIFT);
    }

    const f32_bits out =
        ((u32)signRes << F32_SIGN_SHIFT) |
        ((u32)expRes << F32_EXP_SHIFT) |
        (mantMain & F32_MANT_MASK);
    return f32_from_bits(out);
}

/* Float subtraction */
float __subsf3(float a, float b) {
    f32_bits B = f32_to_bits(b);
    B ^= (1U << F32_SIGN_SHIFT);
    return __addsf3(a, f32_from_bits(B));
}

/* Float multiplication */
float __mulsf3(float a, float b) {
    const f32_bits A = f32_to_bits(a);
    const f32_bits B = f32_to_bits(b);

    if (f32_is_nan(A)) return a;
    if (f32_is_nan(B)) return b;

    const int signA = (int)((A >> F32_SIGN_SHIFT) & 1);
    const int signB = (int)((B >> F32_SIGN_SHIFT) & 1);
    const int sign = signA ^ signB;

    const int expA = (int)((A >> F32_EXP_SHIFT) & F32_EXP_MASK);
    const int expB = (int)((B >> F32_EXP_SHIFT) & F32_EXP_MASK);
    const u32 fracA = (u32)(A & F32_MANT_MASK);
    const u32 fracB = (u32)(B & F32_MANT_MASK);

    if (expA == F32_EXP_MASK) {
        if (fracA) return a;
        if (f32_is_zero(B)) return f32_qnan(); /* inf * 0 */
        return f32_inf(sign);
    }
    if (expB == F32_EXP_MASK) {
        if (fracB) return b;
        if (f32_is_zero(A)) return f32_qnan(); /* 0 * inf */
        return f32_inf(sign);
    }

    if (f32_is_zero(A) || f32_is_zero(B)) {
        return f32_from_bits((u32)sign << F32_SIGN_SHIFT);
    }

    int expUnA = expA ? (expA - F32_BIAS) : (1 - F32_BIAS);
    int expUnB = expB ? (expB - F32_BIAS) : (1 - F32_BIAS);
    u32 mantA = expA ? (F32_IMPLICIT | fracA) : fracA;
    u32 mantB = expB ? (F32_IMPLICIT | fracB) : fracB;

    u64 prod = (u64)mantA * (u64)mantB; /* up to 48 bits */
    int expUnR = expUnA + expUnB;

    /* Normalize so the top bit is at position 46 (value in [1,2)). */
    if (prod & (1ULL << 47)) {
        prod >>= 1;
        expUnR++;
    }

    /* Keep 24+3 bits for rounding: shift right by 20, sticky in bit0. */
    u64 mantExt = prod >> 20; /* 27 bits */
    if (prod & ((1ULL << 20) - 1))
        mantExt |= 1;

    u32 mantMain = (u32)(mantExt >> 3); /* 24 bits */
    const u32 guard = (u32)((mantExt >> 2) & 1);
    const u32 roundb = (u32)((mantExt >> 1) & 1);
    const u32 sticky = (u32)(mantExt & 1);
    if (guard && (roundb || sticky || (mantMain & 1u))) {
        mantMain++;
        if (mantMain == (1u << 24)) {
            mantMain >>= 1;
            expUnR++;
        }
    }

    int expR = expUnR + F32_BIAS;
    if (expR >= F32_EXP_MASK)
        return f32_inf(sign);
    if (expR <= 0)
        return f32_from_bits((u32)sign << F32_SIGN_SHIFT); /* flush */

    const f32_bits out =
        ((u32)sign << F32_SIGN_SHIFT) |
        ((u32)expR << F32_EXP_SHIFT) |
        (mantMain & F32_MANT_MASK);
    return f32_from_bits(out);
}

/* Float division */
float __divsf3(float a, float b) {
    const f32_bits A = f32_to_bits(a);
    const f32_bits B = f32_to_bits(b);

    if (f32_is_nan(A)) return a;
    if (f32_is_nan(B)) return b;

    const int signA = (int)((A >> F32_SIGN_SHIFT) & 1);
    const int signB = (int)((B >> F32_SIGN_SHIFT) & 1);
    const int sign = signA ^ signB;

    const int expA = (int)((A >> F32_EXP_SHIFT) & F32_EXP_MASK);
    const int expB = (int)((B >> F32_EXP_SHIFT) & F32_EXP_MASK);
    const u32 fracA = (u32)(A & F32_MANT_MASK);
    const u32 fracB = (u32)(B & F32_MANT_MASK);

    if (expA == F32_EXP_MASK) {
        if (fracA) return a;
        if (f32_is_inf(B)) return f32_qnan(); /* inf/inf */
        return f32_inf(sign);
    }
    if (expB == F32_EXP_MASK) {
        if (fracB) return b;
        return f32_from_bits((u32)sign << F32_SIGN_SHIFT); /* x/inf = 0 */
    }

    if (f32_is_zero(B)) {
        if (f32_is_zero(A)) return f32_qnan();
        return f32_inf(sign);
    }
    if (f32_is_zero(A)) {
        return f32_from_bits((u32)sign << F32_SIGN_SHIFT);
    }

    int expUnA = expA ? (expA - F32_BIAS) : (1 - F32_BIAS);
    int expUnB = expB ? (expB - F32_BIAS) : (1 - F32_BIAS);
    u32 mantA = expA ? (F32_IMPLICIT | fracA) : fracA;
    u32 mantB = expB ? (F32_IMPLICIT | fracB) : fracB;

    int expUnR = expUnA - expUnB;

    /* Compute 24+3 bits of quotient for rounding. */
    const u64 num = ((u64)mantA) << (23 + 3);
    u64 quot = num / mantB;
    if (num % mantB)
        quot |= 1; /* sticky */

    /* Normalize so implicit bit is at position 23. */
    while (quot < (1ULL << (23 + 3))) {
        quot <<= 1;
        expUnR--;
    }

    u32 mantMain = (u32)(quot >> 3);
    const u32 guard = (u32)((quot >> 2) & 1);
    const u32 roundb = (u32)((quot >> 1) & 1);
    const u32 sticky = (u32)(quot & 1);
    if (guard && (roundb || sticky || (mantMain & 1u))) {
        mantMain++;
        if (mantMain == (1u << 24)) {
            mantMain >>= 1;
            expUnR++;
        }
    }

    int expR = expUnR + F32_BIAS;
    if (expR >= F32_EXP_MASK)
        return f32_inf(sign);
    if (expR <= 0)
        return f32_from_bits((u32)sign << F32_SIGN_SHIFT); /* flush */

    const f32_bits out =
        ((u32)sign << F32_SIGN_SHIFT) |
        ((u32)expR << F32_EXP_SHIFT) |
        (mantMain & F32_MANT_MASK);
    return f32_from_bits(out);
}

/* Float compare */
int __cmpsf2(float a, float b) {
    f32_bits A = *(f32_bits*)&a;
    f32_bits B = *(f32_bits*)&b;
    
    /* Check for NaN */
    if (f32_is_nan(A) || f32_is_nan(B)) {
        return 1; /* Unordered - return 1 */
    }
    
    if (A == B) return 0;
    
    int signA = (A >> F32_SIGN_SHIFT) & 1;
    int signB = (B >> F32_SIGN_SHIFT) & 1;
    
    if (signA != signB) {
        return signA ? -1 : 1;
    }
    
    int expA = (A >> F32_EXP_SHIFT) & F32_EXP_MASK;
    int expB = (B >> F32_EXP_SHIFT) & F32_EXP_MASK;
    u32 mantA = A & F32_MANT_MASK;
    u32 mantB = B & F32_MANT_MASK;
    
    if (expA != expB) {
        return (signA ^ (expA < expB)) ? -1 : 1;
    }
    
    return (signA ^ (mantA < mantB)) ? -1 : 1;
}

/* Float to int conversion */
i32 __fixsfsi(float a) {
    f32_bits A = *(f32_bits*)&a;
    
    int sign = (A >> F32_SIGN_SHIFT) & 1;
    int exp = (A >> F32_EXP_SHIFT) & F32_EXP_MASK;
    u32 mant = A & F32_MANT_MASK;
    
    if (exp == F32_EXP_MASK) {
        /* Infinity or NaN */
        return sign ? 0x80000000 : 0x7FFFFFFF;
    }
    
    if (exp < F32_BIAS) {
        /* |a| < 1 */
        return 0;
    }
    
    int shift = exp - F32_BIAS;
    u64 mant64 = mant | F32_IMPLICIT;
    
    if (shift >= 31) {
        return sign ? 0x80000000 : 0x7FFFFFFF;
    }
    
    u64 result;
    if (shift < 23) {
        result = mant64 >> (23 - shift);
    } else {
        result = mant64 << (shift - 23);
    }
    
    if (sign) {
        return -(i32)result;
    }
    return (i32)result;
}

/* Int to float conversion */
float __floatsisf(i32 a) {
    int sign = 0;
    u32 absA;
    
    if (a < 0) {
        sign = 1;
        absA = (u32)(-(i64)a);
    } else {
        absA = (u32)a;
    }
    
    if (absA == 0) {
        return 0.0f;
    }
    
    /* Find MSB position */
    int exp = 31;
    u32 mant = absA;
    
    if (mant >= (1U << 16)) {
        exp = 47;
        mant >>= 16;
    } else if (mant >= (1U << 8)) {
        exp = 38;
        mant >>= 8;
    } else {
        exp = 30;
        while (mant < (1U << 23)) {
            mant <<= 1;
            exp--;
        }
    }
    
    f32_bits result = (sign << F32_SIGN_SHIFT) | 
                      ((exp + F32_BIAS) << F32_EXP_SHIFT) |
                      (mant & F32_MANT_MASK);
    
    return *(float*)&result;
}

/* Double precision wrappers */
double __adddf3(double a, double b) {
    /* Simplified - just convert to float and back for now */
    return (double)__addsf3((float)a, (float)b);
}

double __subdf3(double a, double b) {
    return (double)__subsf3((float)a, (float)b);
}

double __muldf3(double a, double b) {
    return (double)__mulsf3((float)a, (float)b);
}

double __divdf3(double a, double b) {
    return (double)__divsf3((float)a, (float)b);
}

/* Double/float conversions used by soft-float wrappers.
 *
 * These are required because the compiler will emit calls to them when
 * lowering casts like (float)double and (double)float on soft-float targets.
 *
 * Note: Implemented with simple truncation (no rounding-to-nearest) which is
 * sufficient for the current bring-up tests.
 */
float __truncdfsf2(double a)
{
    f64_bits A = f64_to_bits(a);
    int sign = (int)((A >> F64_SIGN_SHIFT) & 1);
    int exp = (int)((A >> F64_EXP_SHIFT) & F64_EXP_MASK);
    u64 frac = (u64)(A & F64_MANT_MASK);

    if (exp == F64_EXP_MASK) {
        /* NaN/Inf */
        u32 mant = (u32)(frac >> (F64_EXP_SHIFT - F32_EXP_SHIFT));
        if (frac && (mant & F32_MANT_MASK) == 0) {
            mant |= 1; /* keep NaN payload non-zero */
        }
        f32_bits out = ((u32)sign << F32_SIGN_SHIFT) |
                       ((u32)F32_EXP_MASK << F32_EXP_SHIFT) |
                       (mant & F32_MANT_MASK);
        return f32_from_bits(out);
    }

    if (exp == 0) {
        /* Zero or subnormal -> flush to zero for now */
        return f32_from_bits((u32)sign << F32_SIGN_SHIFT);
    }

    int e = exp - F64_BIAS;          /* unbiased exponent */
    int exp_f = e + F32_BIAS;
    if (exp_f >= F32_EXP_MASK) {
        /* Overflow to Inf */
        f32_bits out = ((u32)sign << F32_SIGN_SHIFT) |
                       ((u32)F32_EXP_MASK << F32_EXP_SHIFT);
        return f32_from_bits(out);
    }
    if (exp_f <= 0) {
        /* Underflow -> zero (no subnormals for now) */
        return f32_from_bits((u32)sign << F32_SIGN_SHIFT);
    }

    u64 mant = F64_IMPLICIT | frac; /* 53-bit */
    u32 mant_f = (u32)(mant >> (F64_EXP_SHIFT - F32_EXP_SHIFT)); /* keep top 24 bits */
    f32_bits out = ((u32)sign << F32_SIGN_SHIFT) |
                   ((u32)exp_f << F32_EXP_SHIFT) |
                   (mant_f & F32_MANT_MASK);
    return f32_from_bits(out);
}

double __extendsfdf2(float a)
{
    f32_bits A = f32_to_bits(a);
    int sign = (int)((A >> F32_SIGN_SHIFT) & 1);
    int exp = (int)((A >> F32_EXP_SHIFT) & F32_EXP_MASK);
    u32 frac = (u32)(A & F32_MANT_MASK);

    if (exp == F32_EXP_MASK) {
        /* NaN/Inf */
        f64_bits out = ((u64)sign << F64_SIGN_SHIFT) |
                       ((u64)F64_EXP_MASK << F64_EXP_SHIFT) |
                       ((u64)frac << (F64_EXP_SHIFT - F32_EXP_SHIFT));
        if (frac == 0) {
            /* Inf */
            return f64_from_bits(out);
        }
        /* NaN: ensure mantissa non-zero */
        if ((out & F64_MANT_MASK) == 0) {
            out |= 1;
        }
        return f64_from_bits(out);
    }

    if (exp == 0) {
        /* Zero or subnormal */
        if (frac == 0) {
            return f64_from_bits((u64)sign << F64_SIGN_SHIFT);
        }

        /* Normalize subnormal float */
        int exp_d = (F64_BIAS - F32_BIAS + 1);
        while ((frac & F32_IMPLICIT) == 0) {
            frac <<= 1;
            exp_d--;
        }
        frac &= F32_MANT_MASK;
        f64_bits out = ((u64)sign << F64_SIGN_SHIFT) |
                       ((u64)exp_d << F64_EXP_SHIFT) |
                       ((u64)frac << (F64_EXP_SHIFT - F32_EXP_SHIFT));
        return f64_from_bits(out);
    }

    int exp_d = exp - F32_BIAS + F64_BIAS;
    f64_bits out = ((u64)sign << F64_SIGN_SHIFT) |
                   ((u64)exp_d << F64_EXP_SHIFT) |
                   ((u64)frac << (F64_EXP_SHIFT - F32_EXP_SHIFT));
    return f64_from_bits(out);
}

/* Double comparisons (compiler-rt / libgcc semantics)
 *
 * See LLVM compiler-rt comparedf2.c:
 * - __ltdf2 is an alias of __ledf2: returns 1 on NaN
 * - __gtdf2 is an alias of __gedf2: returns -1 on NaN
 */
static inline int f64_cmp(f64_bits A, f64_bits B, int nan_result)
{
    if (f64_is_nan(A) || f64_is_nan(B)) {
        return nan_result;
    }
    if (f64_is_zero(A) && f64_is_zero(B)) {
        return 0;
    }
    if (A == B) {
        return 0;
    }

    int signA = (int)((A >> F64_SIGN_SHIFT) & 1);
    int signB = (int)((B >> F64_SIGN_SHIFT) & 1);
    if (signA != signB) {
        return signA ? -1 : 1; /* negative < positive */
    }

    if (!signA) {
        return (A < B) ? -1 : 1;
    }
    return (A > B) ? -1 : 1; /* reversed for negatives */
}

/* Single-precision comparisons (compiler-rt / libgcc semantics)
 *
 * See LLVM compiler-rt comparesf2.c:
 * - __ltsf2 is an alias of __lesf2: returns 1 on NaN
 * - __gtsf2 is an alias of __gesf2: returns -1 on NaN
 */
static inline int f32_cmp(f32_bits A, f32_bits B, int nan_result)
{
    if (f32_is_nan(A) || f32_is_nan(B)) {
        return nan_result;
    }
    if (f32_is_zero(A) && f32_is_zero(B)) {
        return 0;
    }
    if (A == B) {
        return 0;
    }

    int signA = (int)((A >> F32_SIGN_SHIFT) & 1);
    int signB = (int)((B >> F32_SIGN_SHIFT) & 1);
    if (signA != signB) {
        return signA ? -1 : 1; /* negative < positive */
    }

    if (!signA) {
        return (A < B) ? -1 : 1;
    }
    return (A > B) ? -1 : 1; /* reversed for negatives */
}

__attribute__((noinline, optnone))
int __ltdf2(double a, double b)
{
    return f64_cmp(f64_to_bits(a), f64_to_bits(b), 1);
}

__attribute__((noinline, optnone))
int __gtdf2(double a, double b)
{
    return f64_cmp(f64_to_bits(a), f64_to_bits(b), -1);
}

__attribute__((noinline, optnone))
int __ledf2(double a, double b)
{
    return f64_cmp(f64_to_bits(a), f64_to_bits(b), 1);
}

__attribute__((noinline, optnone))
int __gedf2(double a, double b)
{
    return f64_cmp(f64_to_bits(a), f64_to_bits(b), -1);
}

__attribute__((noinline, optnone))
int __eqdf2(double a, double b)
{
    return f64_cmp(f64_to_bits(a), f64_to_bits(b), 1);
}

__attribute__((noinline, optnone))
int __nedf2(double a, double b)
{
    return f64_cmp(f64_to_bits(a), f64_to_bits(b), 1);
}

int __unorddf2(double a, double b)
{
    f64_bits A = f64_to_bits(a);
    f64_bits B = f64_to_bits(b);
    return (f64_is_nan(A) || f64_is_nan(B)) ? 1 : 0;
}

__attribute__((noinline, optnone))
int __ltsf2(float a, float b)
{
    return f32_cmp(f32_to_bits(a), f32_to_bits(b), 1);
}

__attribute__((noinline, optnone))
int __gtsf2(float a, float b)
{
    return f32_cmp(f32_to_bits(a), f32_to_bits(b), -1);
}

__attribute__((noinline, optnone))
int __lesf2(float a, float b)
{
    return f32_cmp(f32_to_bits(a), f32_to_bits(b), 1);
}

__attribute__((noinline, optnone))
int __gesf2(float a, float b)
{
    return f32_cmp(f32_to_bits(a), f32_to_bits(b), -1);
}

__attribute__((noinline, optnone))
int __eqsf2(float a, float b)
{
    return f32_cmp(f32_to_bits(a), f32_to_bits(b), 1);
}

__attribute__((noinline, optnone))
int __nesf2(float a, float b)
{
    return f32_cmp(f32_to_bits(a), f32_to_bits(b), 1);
}

int __unordsf2(float a, float b)
{
    f32_bits A = f32_to_bits(a);
    f32_bits B = f32_to_bits(b);
    return (f32_is_nan(A) || f32_is_nan(B)) ? 1 : 0;
}

/* Double to unsigned 64-bit conversion (truncate toward zero). */
u64 __fixunsdfdi(double a)
{
    f64_bits A = f64_to_bits(a);
    int sign = (int)((A >> F64_SIGN_SHIFT) & 1);
    int exp = (int)((A >> F64_EXP_SHIFT) & F64_EXP_MASK);
    u64 frac = (u64)(A & F64_MANT_MASK);

    if (sign) {
        return 0;
    }
    if (exp == F64_EXP_MASK) {
        /* NaN or infinity */
        return frac ? 0 : ~(u64)0;
    }
    if (exp == 0) {
        /* Subnormal or zero => |a| < 1 */
        return 0;
    }

    int e = exp - F64_BIAS;
    if (e < 0) {
        return 0;
    }
    if (e >= 64) {
        return ~(u64)0;
    }

    u64 mant = F64_IMPLICIT | frac;
    if (e > 52) {
        return mant << (e - 52);
    }
    return mant >> (52 - e);
}

/* Double to signed 32-bit conversion (truncate toward zero). */
__attribute__((noinline, optnone))
i32 __fixdfsi(double a)
{
    f64_bits A = f64_to_bits(a);
    int sign = (int)((A >> F64_SIGN_SHIFT) & 1);
    int exp = (int)((A >> F64_EXP_SHIFT) & F64_EXP_MASK);
    u64 frac = (u64)(A & F64_MANT_MASK);

    if (exp == F64_EXP_MASK) {
        /* NaN or infinity */
        return sign ? (i32)0x80000000u : (i32)0x7fffffffu;
    }
    if (exp == 0) {
        /* Subnormal or zero => |a| < 1 */
        return 0;
    }

    int e = exp - F64_BIAS;
    if (e < 0) {
        return 0;
    }
    if (e >= 31) {
        return sign ? (i32)0x80000000u : (i32)0x7fffffffu;
    }

    u64 mant = F64_IMPLICIT | frac;
    u64 abs_val;
    if (e > 52) {
        abs_val = mant << (e - 52);
    } else {
        abs_val = mant >> (52 - e);
    }

    if (sign) {
        if (abs_val >= 0x80000000ull) {
            return (i32)0x80000000u;
        }
        return -(i32)abs_val;
    }
    if (abs_val > 0x7fffffffull) {
        return (i32)0x7fffffffu;
    }
    return (i32)abs_val;
}

static inline int u64_msb_index(u64 x)
{
    int i = 63;
    while (i > 0 && ((x >> i) == 0)) {
        i--;
    }
    return i;
}

/* Signed 32-bit integer to double conversion. */
__attribute__((noinline, optnone))
double __floatsidf(i32 a)
{
    if (a == 0) {
        return 0.0;
    }

    int sign = a < 0;
    u64 abs_val = sign ? (u64)(-(i64)a) : (u64)a;

    int msb = u64_msb_index(abs_val);
    int exp = msb + F64_BIAS;

    /* All i32 values are exactly representable in double. */
    u64 mant = abs_val << (52 - msb);
    u64 frac = mant & F64_MANT_MASK;

    f64_bits out = ((u64)sign << F64_SIGN_SHIFT) |
                   ((u64)exp << F64_EXP_SHIFT) |
                   frac;
    return f64_from_bits(out);
}

/* Unsigned 64-bit integer to double conversion (round-to-nearest-even). */
__attribute__((noinline, optnone))
double __floatundidf(u64 a)
{
    if (a == 0) {
        return 0.0;
    }

    int msb = u64_msb_index(a);
    int exp = msb + F64_BIAS;

    u64 mant;
    if (msb <= 52) {
        mant = a << (52 - msb);
    } else {
        int shift = msb - 52;
        u64 rem_mask = (1ull << shift) - 1ull;
        u64 rem = a & rem_mask;
        u64 half = 1ull << (shift - 1);

        mant = a >> shift; /* top 53 bits */
        if (rem > half || (rem == half && (mant & 1ull))) {
            mant++;
        }

        if (mant == (1ull << 53)) {
            /* Rounded up past 53 bits; renormalize. */
            mant >>= 1;
            exp++;
        }
    }

    u64 frac = mant & F64_MANT_MASK;
    f64_bits out = ((u64)exp << F64_EXP_SHIFT) | frac;
    return f64_from_bits(out);
}

/* Signed 64-bit integer to double conversion (round-to-nearest-even). */
__attribute__((noinline, optnone))
double __floatdidf(i64 a)
{
    if (a == 0) {
        return 0.0;
    }

    int sign = a < 0;
    u64 abs_val;
    if (sign) {
        /* Avoid overflow for INT64_MIN. */
        abs_val = (u64)(-(a + 1)) + 1;
    } else {
        abs_val = (u64)a;
    }

    double mag = __floatundidf(abs_val);
    return sign ? -mag : mag;
}

/* Unsigned 32-bit integer to double conversion. */
__attribute__((noinline, optnone))
double __floatunsidf(u32 a)
{
    return __floatundidf((u64)a);
}
