#ifndef SOCKET_RUNUNISTD_WEBASSEMBLY_UNISTD_H
#define SOCKET_RUNUNISTD_WEBASSEMBLY_UNISTD_H

#if !defined(_UNISTD_H)
#define _UNISTD_H
#endif

#include "stddef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

// TODO(@jwerle): figure out what we want to support here

#ifdef __cplusplus
}
#endif
#endif
