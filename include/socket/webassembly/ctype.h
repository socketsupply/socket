#ifndef SOCKET_RUNTIME_WEBASSEMBLY_CTYPE_H
#define SOCKET_RUNTIME_WEBASSEMBLY_CTYPE_H

#if !defined(_CTYPE_H)
#define _CTYPE_H
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#define EOF -1

#define _tolower(c) (c + 'a' - 'A')
#define _toupper(c) (c + 'A' - 'a')

extern int isascii (int);
extern int isblank (int);
extern int iscntrl (int);
extern int isdigit (int);
extern int isgraph (int);
extern int islower (int);
extern int isprint (int);
extern int isspace (int);
extern int isupper (int);
extern int isxdigit (int);
extern int toascii (int);
extern int tolower (int);
extern int toupper (int);

#if defined(__cplusplus)
}
#endif
#endif
