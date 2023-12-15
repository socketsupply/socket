#ifndef SOCKET_RUNTIME_WEBASSEMBLY_STDIO_H
#define SOCKET_RUNTIME_WEBASSEMBLY_STDIO_H

#if !defined(_STDIO_H)
#define _STDIO_H
#endif

#include "string.h"
#include "stdint.h"

#if defined(BUFSIZ)
#undef BUFSIZ
#endif

#define BUFSIZ 1024

#if defined(__cplusplus)
extern "C" {
#endif

struct _FILE { void* _internal; };
typedef struct _FILE FILE;

static FILE* stdin = (FILE*) 0x0;
static FILE* stdout = (FILE*) 0x1;
static FILE* stderr = (FILE*) 0x2;

extern "C" int printf (const char* __restrict format, ...);
extern "C" int fprintf (FILE* __restrict stream, const char* __restrict format, ...);
extern "C" int sprintf (char* __restrict str, const char* __restrict format, ...);
extern "C" int snprintf (char* __restrict str, size_t size, const char* __restrict format, ...);

#if defined(__cplusplus)
}
#endif
#endif
