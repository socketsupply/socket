#ifndef SOCKET_RUNTIME_WEBASSEMBLY_STDLIB_H
#define SOCKET_RUNTIME_WEBASSEMBLY_STDLIB_H

#if !defined(_STDLIB_H)
#define _STDLIB_H
#endif

#include "stddef.h"
#include "stdint.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0
#define RAND_MAX (0x7fffffff)

#define WEXITSTATUS(s) (((s) & 0xff00) >> 8)
#define WTERMSIG(s) ((s) & 0x7f)
#define WSTOPSIG(s) WEXITSTATUS(s)
#define WIFEXITED(s) (!WTERMSIG(s))
#define WIFSTOPPED(s) ((short)((((s)&0xffff)*0x10001)>>8) > 0x7f00)
#define WIFSIGNALED(s) (((s)&0xffff)-1U < 0xffu)

extern int atoi (const char*);
extern long atol (const char*);
extern long long atoll (const char*);
extern double atof (const char*);

extern float strtof (const char* __restrict, char** __restrict);
extern double strtod (const char* __restrict, char** __restrict);
extern long double strtold (const char* __restrict, char** __restrict);

extern long strtol (const char* __restrict, char** __restrict, int);
extern unsigned long strtoul (const char* __restrict, char** __restrict, int);
extern long long strtoll (const char* __restrict, char* *__restrict, int);
extern unsigned long long strtoull (const char* __restrict, char** __restrict, int);

extern int rand (void);
extern void srand (unsigned);

extern void* malloc (size_t);
extern void* calloc (size_t, size_t);
extern void* realloc (void*, size_t);
extern void free (void*);

extern void abort (void);
extern int atexit (void (*) (void));
extern void exit (int);
extern void _Exit (int);
extern int at_quick_exit (void (*) (void));
extern void quick_exit (int);

extern char *getenv (const char*);
extern int setenv (const char*, const char*, int);
extern int unsetenv (const char*);
extern int putenv (char *);
extern int clearenv (void) WASM_NOOP;
char* secure_getenv (const char*) WASM_ALIAS(getenv);

extern int system (const char*) WASM_NOOP;

extern int abs (int);
extern long labs (long);
extern long long llabs (long long);

typedef struct { int quot, rem; } div_t;
typedef struct { long quot, rem; } ldiv_t;
typedef struct { long long quot, rem; } lldiv_t;

extern div_t div (int, int);
extern ldiv_t ldiv (long, long);
extern lldiv_t lldiv (long long, long long);

#if defined(__cplusplus)
}
#endif
#endif
