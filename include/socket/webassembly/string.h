#ifndef SOCKET_RUNTIME_WEBASSEMBLY_STRING_H
#define SOCKET_RUNTIME_WEBASSEMBLY_STRING_H

#if !defined(_STRING_H)
#define _STRING_H
#endif

#include "features.h"
#include "stddef.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern void* memcpy (void* __restrict, const void* __restrict, size_t);
extern void* memmove (void*, const void*, size_t);
extern void* memset (void*, int, size_t);
extern int memcmp (const void*, const void*, size_t);
extern void* memchr (const void*, int, size_t);
extern char* strcpy (char* __restrict, const char* __restrict);
extern char* strncpy (char* __restrict, const char* __restrict, size_t);
extern char* strcat (char* __restrict, const char* __restrict);
extern char* strncat (char* __restrict, const char* __restrict, size_t);
extern int strcmp (const char*, const char*);
extern int strncmp (const char*, const char*, size_t);
extern int strcoll (const char*, const char*);
extern size_t strxfrm (char* __restrict, const char* __restrict, size_t);
extern char* strchr (const char*, int);
extern char* strrchr (const char*, int);
extern char* stpcpy (char* __restrict, const char* __restrict);
extern char* stpncpy (char* __restrict, const char* __restrict, size_t);
extern size_t strnlen (const char*, size_t);
extern char* strdup (const char*);
extern char* strndup (const char*, size_t);
extern char* strsignal (int);
extern char* strerror (int);
extern size_t strcspn (const char*, const char*);
extern size_t strspn (const char*, const char*);
extern char* strpbrk (const char*, const char*);
extern char* strstr (const char*, const char *);
extern char* strtok (char* __restrict, const char* __restrict);
extern size_t strlen (const char*);

#if defined(__cplusplus)
}
#endif
#endif
