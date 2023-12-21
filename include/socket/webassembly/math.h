#ifndef SOCKET_RUNMATH_WEBASSEMBLY_MATH_H
#define SOCKET_RUNMATH_WEBASSEMBLY_MATH_H

#if !defined(_MATH_H)
#define _MATH_H
#endif

#include "stddef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NAN (0.0f/0.0f)
#define INFINITY 1e5000f

#define HUGE_VALF INFINITY
#define HUGE_VAL ((double) INFINITY)
#define HUGE_VALL ((double) INFINITY)

#define MATH_ERRNO 1
#define MATH_ERREXCEPT 2
#define math_errhandling 2

#define FP_ILOGBNAN (-1-0x7fffffff)
#define FP_ILOGB0 FP_ILOGBNAN

#define FP_NAN 0
#define FP_INFINITE 1
#define FP_ZERO 2
#define FP_SUBNORMAL 3
#define FP_NORMAL 4

#define M_E 2.7182818284590452354          // e
#define M_LOG2E 1.4426950408889634074      // log_2 e
#define M_LOG10E 0.43429448190325182765    // log_10 e
#define M_LN2 0.69314718055994530942       // log_e 2
#define M_LN10 2.30258509299404568402      // log_e 10
#define M_PI 3.14159265358979323846        // pi
#define M_PI_2 1.57079632679489661923      // pi/2
#define M_PI_4 0.78539816339744830962      // pi/4
#define M_1_PI 0.31830988618379067154      // 1/pi
#define M_2_PI 0.63661977236758134308      // 2/pi
#define M_2_SQRTPI 1.12837916709551257390  // 2/sqrt(pi)
#define M_SQRT2 1.41421356237309504880     // sqrt(2)
#define M_SQRT1_2 0.70710678118654752440   // 1/sqrt(2)

// TODO(@jwerle): figure out what we want to support here

#define fpclassify(x)  (                                                       \
  __builtin_fpclassify (                                                       \
    FP_NAN,                                                                    \
    FP_INFINITE,                                                               \
    FP_NORMAL,                                                                 \
    FP_SUBNORMAL,                                                              \
    FP_ZERO,                                                                   \
    x                                                                          \
  )                                                                            \
)

#define isinf(x) (__builtin_isinf(x))
#define isnan(x) (__builtin_isnan(x))
#define isnormal(x) (__builtin_isnormal(x))
#define isfinite(x) (__builtin_isfinite(x))
#define signbit(x) (__builtin_signbit(x))
#define isunordered(x, y) (__builtin_isunordered(x, y))
#define isless(x, y) (__builtin_isless(x, y))
#define islessequal(x, y) (__builtin_islessequal(x, y))
#define islessgreater(x, y) (__builtin_islessgreater(x, y))
#define isgreater(x, y) (__builtin_isgreater(x, y))
#define isgreaterequal(x, y) (__builtin_isgreaterequal(x, y))

/**
 * @notice 128 bit (16 bytes) floating point numbers (long double) are not
 * supported. The `*l()` functions accept/return `double` instead.
 *
 * @TODO(@jwerle): figure out how to implement https://www.davidhbailey.com/dhbpapers/quad-double.pdf
 */

extern double acos (double);
extern float acosf (float);
extern double acosl (double);
extern double acosh (double);
extern float acoshf (float);
extern double acoshl (double);
extern double asin (double);
extern float asinf (float);
extern double asinl (double);
extern double asinh (double);
extern float asinhf (float);
extern double asinhl (double);
extern double atan (double);
extern float atanf (float);
extern double atanl (double);
extern double atan2 (double, double);
extern float atan2f (float, float);
extern double atan2l (double, double);
extern double atanh (double);
extern float atanhf (float);
extern double atanhl (double);
extern double cbrt (double);
extern float cbrtf (float);
extern double cbrtl (double);
extern double ceil (double);
extern float ceilf (float);
extern double ceill (double);
extern double copysign (double, double);
extern float copysignf (float, float);
extern double copysignl (double, double);
extern double cos (double);
extern float cosf (float);
extern double cosl (double);
extern double cosh (double);
extern float coshf (float);
extern double coshl (double);
extern double erf (double);
extern float erff (float);
extern double erfl (double);
extern float erfcf (float);
extern double erfcl (double);
extern double exp (double);
extern float expf (float);
extern double expl (double);
extern double exp2 (double);
extern float exp2f (float);
extern double exp2l (double);
extern double expm1 (double);
extern float expm1f (float);
extern double expm1l (double);
extern double fabs (double);
extern float fabsf (float);
extern double fabsl (double);
extern double fdim (double, double);
extern float fdimf (float, float);
extern double fdiml (double, double);
extern double floor (double);
extern float floorf (float);
extern double floorl (double);
extern double fma (double, double, double);
extern float fmaf (float, float, float);
extern double fmal (double, double, double);
extern double fmax (double, double);
extern float fmaxf (float, float);
extern double fmaxl (double, double);
extern double fmin (double, double);
extern float fminf (float, float);
extern double fminl (double, double);
extern double fmod (double, double);
extern float fmodf (float, float);
extern double fmodl (double, double);
extern double frexp (double, int*);
extern float frexpf (float, int*);
extern double frexpl (double, int*);
extern double hypot (double, double);
extern float hypotf (float, float);
extern double hypotl (double, double);
extern int ilogb (double);
extern int ilogbf (float);
extern int ilogbl (double);
extern double ldexp (double, int);
extern float ldexpf (float, int);
extern double ldexpl (double, int);
extern double lgamma (double);
extern float lgammaf (float);
extern double lgammal (double);
extern long long llrint (double);
extern long long llrintf (float);
extern long long llrintl (double);
extern long long llround (double);
extern long long llroundf (float);
extern long long llroundl (double);
extern double log (double);
extern float logf (float);
extern double logl (double);
extern double log10 (double);
extern float log10f (float);
extern double log10l (double);
extern double log1p (double);
extern float log1pf (float);
extern double log1pl (double);
extern double log2 (double);
extern float log2f (float);
extern double log2l (double);
extern double logb (double);
extern float logbf (float);
extern double logbl (double);
extern long lrint (double);
extern long lrintf (float);
extern long lrintl (double);
extern long lround (double);
extern long lroundf (float);
extern long lroundl (double);
extern double modf (double, double*);
extern float modff (float, float*);
extern double modfl (double, double*);
extern double nearbyint (double);
extern float nearbyintf (float);
extern double nearbyintl (double);
extern double nextafter (double, double);
extern float nextafterf (float, float);
extern double nextafterl (double, double);
extern double nexttoward (double, double);
extern float nexttowardf (float, double);
extern double nexttowardl (double, double);
extern double pow (double, double);
extern float powf (float, float);
extern double powl (double, double);
extern double remainder (double, double);
extern float remainderf (float, float);
extern double remainderl (double, double);
extern double remquo (double, double, int*);
extern float remquof (float, float, int*);
extern double remquol (double, double, int*);
extern double rint (double);
extern float rintf (float);
extern double rintl (double);
extern double round (double);
extern float roundf (float);
extern double roundl (double);
extern double scalbln (double, long);
extern float scalblnf (float, long);
extern double scalblnl (double, long);
extern double scalbn (double, int);
extern float scalbnf (float, int);
extern double scalbnl (double, int);
extern double sin (double);
extern float sinf (float);
extern double sinl (double);
extern double sinh (double);
extern float sinhf (float);
extern double sinhl (double);
extern double sqrt (double);
extern float sqrtf (float);
extern double sqrtl (double);
extern double tan (double);
extern float tanf (float);
extern double tanl (double);
extern double tanh (double);
extern float tanhf (float);
extern double tanhl (double);
extern double tgamma (double);
extern float tgammaf (float);
extern double tgammal (double);
extern double trunc (double);
extern float truncf (float);
extern double truncl (double);

extern int signgam;

#ifdef __cplusplus
}
#endif
#endif
