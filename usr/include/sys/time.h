#pragma once

#include <stdint.h>
#include <stddef.h>
#include <types.h>

struct timespec {
    time_t tv_sec;
    long   tv_nsec;
};

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
};

typedef struct timeval {
    time_t          tv_sec;  // Seconds.
    suseconds_t    tv_usec; // Microseconds.
} timeval_t;
int gettimeofday(struct timeval *restrict tp, void *restrict tzp);

struct timezone
{
    int tz_minuteswest; /* minutes west of Greenwich */
    int tz_dsttime;     /* type of DST correction */
};

extern int settimeofday(struct timeval *p, void *z);

clock_t clock(void);


struct tm *localtime(const time_t *timer);
struct tm *localtime_r(const time_t *restrict timer,
                       struct tm *restrict result);

time_t mktime(struct tm *timeptr);

int clock_getres(clockid_t clock_id, struct timespec *res);
int clock_gettime(clockid_t clock_id, struct timespec *tp);
int clock_settime(clockid_t clock_id, const struct timespec *tp);