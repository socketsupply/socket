#ifndef SOCKET_RUNTIME_WEBASSEMBLY_LIBGEN_H
#define SOCKET_RUNTIME_WEBASSEMBLY_LIBGEN_H

#if !defined(_LIBGEN_H)
#define _LIBGEN_H
#endif

#if defined(__cplusplus)
extern "C" {
#endif

extern char* basename (const char* path);
extern char* dirname (const char* path);

#if defined(__cplusplus)
}
#endif
#endif
