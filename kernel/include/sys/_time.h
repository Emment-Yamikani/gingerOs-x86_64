#pragma once

#include <lib/stdint.h>
#include <lib/stddef.h>
#include <lib/types.h>

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
    susseconds_t    tv_usec; // Microseconds.
} timeval_t;

#define TIMESPEC_TO_TIMEVAL(ts, tv) ({           \
    do                                        \
    {                                         \
        (tv)->tv_sec = (ts)->tv_sec;          \
        (tv)->tv_usec = (ts)->tv_nsec / 1000; \
    } while (0);                              \
})

#define TIMEVAL_TO_TIMESPEC(tv, ts) ({        \
    do                                        \
    {                                         \
        (ts)->tv_sec = (tv)->tv_sec;          \
        (ts)->tv_nsec = (tv)->tv_usec * 1000; \
    } while (0);                              \
})

#define TIMESPEC_ADD(ts1, ts2) ({               \
    timespec_t result;                          \
    result.tv_sec = ts1.tv_sec + ts2.tv_sec;    \
    result.tv_nsec = ts1.tv_nsec + ts2.tv_nsec; \
    if (result.tv_nsec >= 1000000000)           \
    {                                           \
        result.tv_sec++;                        \
        result.tv_nsec -= 1000000000;           \
    }                                           \
    result;                                     \
})

#define TIMESPEC_SUB(ts1, ts2) ({               \
    timespec_t result;                          \
    result.tv_sec = ts1.tv_sec - ts2.tv_sec;    \
    result.tv_nsec = ts1.tv_nsec - ts2.tv_nsec; \
    if (result.tv_nsec < 0)                     \
    {                                           \
        result.tv_sec--;                        \
        result.tv_nsec += 1000000000;           \
    }                                           \
    result;                                     \
})

#define TIMESPEC_EQ(ts1, ts2) ({            \
    ((ts1)->tv_sec == (ts2)->tv_sec) &&     \
        ((ts1)->tv_nsec == (ts2)->tv_nsec); \
})

#define TIMEVAL_ADD(tv1, tv2) ({                \
    timeval_t result;                           \
    result.tv_sec = tv1.tv_sec + tv2.tv_sec;    \
    result.tv_usec = tv1.tv_usec + tv2.tv_usec; \
    if (result.tv_usec >= 1000000)              \
    {                                           \
        result.tv_sec++;                        \
        result.tv_usec -= 1000000;              \
    }                                           \
    result;                                     \
})

#define TIMEVAL_SUB(tv1, tv2) ({                \
    timeval_t result;                           \
    result.tv_sec = tv1.tv_sec - tv2.tv_sec;    \
    result.tv_usec = tv1.tv_usec - tv2.tv_usec; \
    if (result.tv_usec < 0)                     \
    {                                           \
        result.tv_sec--;                        \
        result.tv_usec += 1000000;              \
    }                                           \
    result;                                     \
})

#define TIMEVAL_EQ(tv1, tv2) ({             \
    ((tv1)->tv_sec == (tv2)->tv_sec) &&     \
        ((tv1)->tv_usec == (tv2)->tv_usec); \
})

int gettimeofday(struct timeval *restrict tp, void *restrict tzp);

clock_t clock(void);

struct tm *localtime(const time_t *timer);
struct tm *localtime_r(const time_t *restrict timer,
                       struct tm *restrict result);

time_t mktime(struct tm *timeptr);

int clock_getres(clockid_t clock_id, struct timespec *res);
int clock_gettime(clockid_t clock_id, struct timespec *tp);
int clock_settime(clockid_t clock_id, const struct timespec *tp);
