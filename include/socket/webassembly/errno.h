#ifndef SOCKET_RUNTIME_WEBASSEMBLY_ERRNO_H
#define SOCKET_RUNTIME_WEBASSEMBLY_ERRNO_H

#if !defined(_ERRNO_H)
#define _ERRNO_H
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#include "sys/errno.h"

int *__errno_location(void);
#define errno (*__errno_location())

#if defined(__cplusplus)
}
#endif
#endif
