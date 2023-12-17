#ifndef SOCKET_RUNTIME_WEBASSEMBLY_TIME_H
#define SOCKET_RUNTIME_WEBASSEMBLY_TIME_H

#if !defined(_TIME_H)
#define _TIME_H
#endif

#include "features.h"
#include "stddef.h"
#include "stdint.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define CLOCKS_PER_SEC 1000

#define TIME_UTC 1
#define TIMER_ABSTIME 1

#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1
#define CLOCK_PROCESS_CPUTIME_ID 2
#define CLOCK_THREAD_CPUTIME_ID 3
#define CLOCK_MONOTONIC_RAW 4
#define CLOCK_REALTIME_COARSE 5
#define CLOCK_MONOTONIC_COARSE 6
#define CLOCK_BOOTTIME 7
#define CLOCK_REALTIME_ALARM 8
#define CLOCK_BOOTTIME_ALARM 9
#define CLOCK_SGI_CYCLE 10
#define CLOCK_TAI 11

typedef unsigned int clock_t;
typedef int32_t time_t;
typedef int clockid_t;

struct tm {
  int tm_sec;
  int tm_min;
  int tm_hour;
  int tm_mday;
  int tm_mon;
  int tm_year;
  int tm_wday;
  int tm_yday;
  int tm_isdst;
  long __tm_gmtoff;
  const char *__tm_zone;
};

struct timespec {
  time_t tv_sec;  // seconds
  long tv_nsec; // nanoseconds
};

struct itimerspec {
  struct timespec it_interval;
  struct timespec it_value;
};

extern time_t time (time_t*);
extern double difftime (time_t, time_t);
extern time_t mktime (struct tm*);
extern size_t strftime (
  char* __restrict,
  size_t,
  const char* __restrict,
  const struct tm* __restrict
);

extern char* strptime (
  const char* __restrict,
  const char* __restrict,
  struct tm* __restrict
);

extern struct tm *gmtime (const time_t*);
extern time_t timegm (struct tm*);

extern struct tm *localtime (const time_t*);
extern char *asctime (const struct tm*);
extern char *ctime (const time_t*);
extern int stime (const time_t*);

extern int timespec_get (struct timespec*, int);
extern struct tm* getdate (const char*);

extern int nanosleep (const struct timespec*, struct timespec*);

extern struct tm* gmtime_r (const time_t* __restrict, struct tm* __restrict);
extern struct tm* localtime_r (const time_t* __restrict, struct tm* __restrict);
extern char* asctime_r (const struct tm* __restrict, char* __restrict);
extern char* ctime_r (const time_t*, char*);

// clock
extern clock_t clock (void);
extern int clock_getres (clockid_t, struct timespec *);
extern int clock_gettime (clockid_t, struct timespec *);
extern int clock_nanosleep (clockid_t, int, const struct timespec*, struct timespec*);

#if defined(__cplusplus)
}
#endif
#endif
