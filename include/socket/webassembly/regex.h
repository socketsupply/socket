#ifndef SOCKET_RUNTIME_WEBASSEMBLY_REGEX_H
#define SOCKET_RUNTIME_WEBASSEMBLY_REGEX_H

#if !defined(_REGEX_H)
#define _REGEX_H
#endif

#include "stddef.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define REG_EXTENDED 0x1
#define REG_ICASE 0x2
#define REG_NOSUB 0x4
#define REG_NEWLINE 0x8

#define REG_NOMATCH 1
#define REG_BADPAT 2
#define REG_ECOLLATE 3
#define REG_ECTYPE 4
#define REG_EESCAPE 5
#define REG_ESUBREG 6
#define REG_EBRACK 7
#define REG_EPAREN 8
#define REG_EBRACE 9
#define REG_BADBR 10
#define REG_ERANGE 11
#define REG_ESPACE 12
#define REG_BADRPT 13
#define REG_ENOSYS 14

#define REG_NOTBOL 0x1
#define REG_NOTEOL 0x2

typedef long regoff_t;

typedef struct {
  size_t re_nsub;
  void* re_pcre;
} regex_t;

typedef struct {
  regoff_t rm_so;
  regoff_t rm_eo;
} regmatch_t;

extern int regcomp (regex_t* regex, const char* pattern, int flags);
extern size_t regerror (
  int error,
  const regex_t* regex,
  char* buf,
  size_t buf_cnt
);

extern int regexec (
  const regex_t* regex,
  const char* string,
  size_t match_cnt,
  regmatch_t match[],
  int flags
);

extern void regfree (regex_t* regex);

#if defined(__cplusplus)
}
#endif
#endif
