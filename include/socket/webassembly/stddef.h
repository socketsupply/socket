#ifndef SOCKET_RUNTIME_WEBASSEMBLY_STDDEF_H
#define SOCKET_RUNTIME_WEBASSEMBLY_STDDEF_H

#if !defined(_STDDEF_H)
#define _STDDEF_H
#endif

#include "stdint.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define WASM_NOOP
#define WASM_ALIAS(name)

#define offsetof(type, member) ((size_t)((char*) &(((type*) 0)->member) - (char *) 0))

#if defined(NULL)
#undef NULL
#endif

#if defined(__cplusplus)
#  if __cplusplus >= 201103L
#    define NULL nullptr
#  elif defined(__cplusplus)
#    define NULL 0L
#  else
#  endif
#else
#  define NULL ((void*)0)
#endif

typedef unsigned long size_t;
typedef long ssize_t;
typedef long ptrdiff_t;
typedef double max_align_t;

typedef int32_t wint_t;
// typedef int32_t wchar_t;

#define INT8_MIN (-1-0x7f)
#define INT16_MIN (-1-0x7fff)
#define INT32_MIN (-1-0x7fffffff)
#define INT64_MIN (-1-0x7fffffffffffffff)

#define INT8_MAX (0x7f)
#define INT16_MAX (0x7fff)
#define INT32_MAX (0x7fffffff)
#define INT64_MAX (0x7fffffffffffffff)

#define UINT8_MAX (0xff)
#define UINT16_MAX (0xffff)
#define UINT32_MAX (0xffffffffu)
#define UINT64_MAX (0xffffffffffffffffu)

#define INT_FAST8_MIN INT8_MIN
#define INT_FAST64_MIN INT64_MIN

#define INT_LEAST8_MIN INT8_MIN
#define INT_LEAST16_MIN INT16_MIN
#define INT_LEAST32_MIN INT32_MIN
#define INT_LEAST64_MIN INT64_MIN

#define INT_FAST8_MAX INT8_MAX
#define INT_FAST64_MAX INT64_MAX

#define INT_LEAST8_MAX INT8_MAX
#define INT_LEAST16_MAX INT16_MAX
#define INT_LEAST32_MAX INT32_MAX
#define INT_LEAST64_MAX INT64_MAX

#define UINT_FAST8_MAX  UINT8_MAX
#define UINT_FAST64_MAX UINT64_MAX

#define UINT_LEAST8_MAX UINT8_MAX
#define UINT_LEAST16_MAX UINT16_MAX
#define UINT_LEAST32_MAX UINT32_MAX
#define UINT_LEAST64_MAX UINT64_MAX

#define INTMAX_MIN INT64_MIN
#define INTMAX_MAX INT64_MAX
#define UINTMAX_MAX UINT64_MAX

#define SIG_ATOMIC_MIN INT32_MIN
#define SIG_ATOMIC_MAX INT32_MAX

#define WINT_MIN 0U
#define WINT_MAX UINT32_MAX

#if L'\0'-1 > 0
#define WCHAR_MAX (0xffffffffu+L'\0')
#define WCHAR_MIN (0+L'\0')
#else
#define WCHAR_MAX (0x7fffffff+L'\0')
#define WCHAR_MIN (-1-0x7fffffff+L'\0')
#endif

#define INT8_C(c)  c
#define INT16_C(c) c
#define INT32_C(c) c

#define UINT8_C(c)  c
#define UINT16_C(c) c
#define UINT32_C(c) c ## U

#if UINTPTR_MAX == UINT64_MAX
#define INT64_C(c) c ## L
#define UINT64_C(c) c ## UL
#define INTMAX_C(c)  c ## L
#define UINTMAX_C(c) c ## UL
#else
#define INT64_C(c) c ## LL
#define UINT64_C(c) c ## ULL
#define INTMAX_C(c)  c ## LL
#define UINTMAX_C(c) c ## ULL
#endif

#if defined(__cplusplus)
}
#endif

#endif
