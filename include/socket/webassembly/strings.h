#ifndef SOCKET_RUNTIME_WEBASSEMBLY_STRINGS_H
#define SOCKET_RUNTIME_WEBASSEMBLY_STRINGS_H

#if !defined(_STRINGS_H)
#define _STRINGS_H
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#include "stddef.h"

extern int bcmp (const void*, const void*, size_t);
extern void bcopy (const void*, void*, size_t);
extern void bzero (void*, size_t);
extern int ffs (int);
extern char* index (const char*, int);
extern char* rindex (const char*, int);
extern int strcasecmp (const char*, const char*);
extern int strncasecmp (const char*, const char*, size_t);

#if defined(__cplusplus)
}
#endif
#endif
