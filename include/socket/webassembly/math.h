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
#define HUGE_VAL ((double)INFINITY)
#define HUGE_VALL ((long double)INFINITY)

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

// TODO(@jwerle): figure out what we want to support here

#ifdef __cplusplus
}
#endif
#endif
