#ifndef SOCKET_RUNTIME_WEBASSEMBLY_CTYPE_H
#define SOCKET_RUNTIME_WEBASSEMBLY_CTYPE_H

#if !defined(_CTYPE_H)
#define _CTYPE_H
#endif

#define EOF -1

#define _tolower(c) (c + 'a' - 'A')
#define _toupper(c) (c + 'A' - 'a')

extern int isascii (int char);
extern int isblank (int char);
extern int iscntrl (int char);
extern int isdigit (int char);
extern int isgraph (int char);
extern int islower (int char);
extern int isprint (int char);
extern int isspace (int char);
extern int isupper (int char);
extern int isxdigit (int char);
extern int toascii (int char);
extern int tolower (int char);
extern int toupper (int char);

#endif
