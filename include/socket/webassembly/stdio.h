#ifndef SOCKET_RUNTIME_WEBASSEMBLY_STDIO_H
#define SOCKET_RUNTIME_WEBASSEMBLY_STDIO_H

#if !defined(_STDIO_H)
#define _STDIO_H
#endif

#include "string.h"
#include "stdint.h"
#include "unistd.h"

#if defined(BUFSIZ)
#undef BUFSIZ
#endif

#define BUFSIZ 4096

#if defined(__cplusplus)
extern "C" {
#endif

struct _FILE { void* _internal; };
typedef struct _FILE FILE;

static FILE* stdin = (FILE*) STDIN_FILENO;
static FILE* stdout = (FILE*) STDOUT_FILENO;
static FILE* stderr = (FILE*) STDERR_FILENO;

extern "C" int printf (const char* __restrict format, ...);
extern "C" int fprintf (FILE* __restrict stream, const char* __restrict format, ...);
extern "C" int sprintf (char* __restrict str, const char* __restrict format, ...);
extern "C" int snprintf (char* __restrict str, size_t size, const char* __restrict format, ...);

#if defined(__cplusplus)
}
#endif
#endif
