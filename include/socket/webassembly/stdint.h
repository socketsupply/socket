#ifndef SOCKET_RUNTIME_WEBASSEMBLY_STDINT_H
#define SOCKET_RUNTIME_WEBASSEMBLY_STDINT_H

#if !defined(_STDINT_H)
#define _STDINT_H
#endif

// Exact-width integer types
typedef unsigned char uint8_t;
typedef uint8_t uint_least8_t;
typedef uint8_t uint_fast8_t;

typedef signed char int8_t;
typedef int8_t int_least8_t;
typedef int8_t int_fast8_t;

typedef unsigned short uint16_t;
typedef uint16_t uint_least16_t;
typedef uint16_t uint_fast16_t;

typedef signed short int16_t;
typedef int16_t int_least16_t;
typedef int16_t int_fast16_t;

typedef unsigned int uint32_t;
typedef uint32_t uint_least32_t;
typedef uint32_t uint_fast32_t;

typedef signed int int32_t;
typedef int32_t int_least32_t;
typedef int32_t int_fast32_t;

#if defined(SOCKET_RUNTIME_EXTENSION_WASM64)
typedef unsigned long long uint64_t;
typedef uint64_t uint_least64_t;
typedef uint64_t uint_fast64_t;

typedef signed long long int64_t;
typedef int64_t int_least64_t;
typedef int64_t int_fast64_t;
#else
typedef uint32_t uint64_t;
typedef uint32_t uint_least64_t;
typedef uint32_t uint_fast64_t;

typedef int32_t int64_t;
typedef int32_t int_least64_t;
typedef int32_t int_fast64_t;
#endif

typedef int32_t intptr_t;
typedef uint32_t uintptr_t;

typedef int32_t intmax_t;
typedef uint32_t uintmax_t;

#define INT_FAST16_MIN INT32_MIN
#define INT_FAST32_MIN INT32_MIN

#define INT_FAST16_MAX INT32_MAX
#define INT_FAST32_MAX INT32_MAX

#define UINT_FAST16_MAX UINT32_MAX
#define UINT_FAST32_MAX UINT32_MAX

#define INTPTR_MIN INT32_MIN
#define INTPTR_MAX INT32_MAX
#define UINTPTR_MAX UINT32_MAX
#define PTRDIFF_MIN INT32_MIN
#define PTRDIFF_MAX INT32_MAX
#define SIZE_MAX UINT32_MAX

#endif
