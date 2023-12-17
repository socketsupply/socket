#ifndef SOCKET_RUNULIMIT_WEBASSEMBLY_ULIMIT_H
#define SOCKET_RUNULIMIT_WEBASSEMBLY_ULIMIT_H

#if !defined(_ULIMIT_H)
#define _ULIMIT_H
#endif

#include "stddef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UL_GETFSIZE 1
#define UL_SETFSIZE 2

extern long ulimit (int, ...) WASM_NOOP;

#ifdef __cplusplus
}
#endif
#endif
