#ifndef SOCKET_RUNTIME_WEBASSEMBLY_ASSERT_H
#define SOCKET_RUNTIME_WEBASSEMBLY_ASSERT_H

#include "features.h"

#if defined(assert)
#undef assert
#endif

#define assert(x) ((void)((x) || (__assert_fail(#x, __FILE__, __LINE__, __func__), 0)))

#if __STDC_VERSION__ >= 201112L && !defined(__cplusplus)
#define static_assert _Static_assert
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern void __assert_fail (const char*, const char*, int, const char*);

#ifdef __cplusplus
}
#endif
#endif
