#pragma once

#include <lib/stdint.h>
#include <lib/stddef.h>
#include <lib/types.h>
#include <sys/_time.h>

// The system heartbeat @ 100Hz = 10000000ns
#define SYS_HZ              (100)

#define s_TO_ms(s)          ((double)(s) * 1000ul)        // convert seconds to ms
#define s_TO_us(s)          ((double)(s) * 1000000ul)     // convert seconds to us
#define s_TO_ns(s)          ((double)(s) * 1000000000ul)  // convert seconds to ns

#define ms_TO_s(s)          ((double)(s) / 1000ul)        // convert ms to seconds
#define us_TO_s(s)          ((double)(s) / 1000000ul)     // convert us to seconds
#define ns_TO_s(s)          ((double)(s) / 1000000000ul)  // convert ns to seconds

#define HZ_TO_s(Hz)         ((double)1 / (Hz))          // convert Hz to s
#define HZ_TO_ms(Hz)        (s_TO_ms(HZ_TO_s(Hz)))      // convert Hz to ms
#define HZ_TO_us(Hz)        (s_TO_us(HZ_TO_s(Hz)))      // convert Hz to us
#define HZ_TO_ns(Hz)        (s_TO_ns(HZ_TO_s(Hz)))      // convert Hz to ns

#define NSEC_PER_USEC       (1000)
#define USEC_PER_SEC        (1000000)
#define NSEC_PER_SEC        (NSEC_PER_USEC * 1000)

#define JIFFIES_TO_TIMEVAL(jiffies, tv) ({       \
    do                                           \
    {                                            \
        ulong __secs = (jiffies) / SYS_HZ;       \
        ulong __usecs = ((jiffies) % SYS_HZ) *   \
                        (USEC_PER_SEC / SYS_HZ); \
        (tv)->tv_sec = __secs;                   \
        (tv)->tv_usec = __usecs;                 \
    } while (0);                                 \
})

#define JIFFIES_TO_TIMESPEC(jiffies, ts) ({      \
    do                                           \
    {                                            \
        ulong __secs = (jiffies) / SYS_HZ;       \
        ulong __nsecs = ((jiffies) % SYS_HZ) *   \
                        (NSEC_PER_SEC / SYS_HZ); \
        (ts)->tv_sec = __secs;                   \
        (ts)->tv_nsecs = __nsecs;                \
    } while (0);                                 \
})

#define TIMESPEC_TO_JIFFIES(ts) ({               \
    (((ts)->tv_sec * SYS_HZ) +                   \
     ((ts)->tv_nsec / (NSEC_PER_SEC / SYS_HZ))); \
})

#define TIMEVAL_TO_JIFFIES(tv) ({                \
    (((tv)->tv_sec * SYS_HZ) +                   \
     ((tv)->tv_usec / (USEC_PER_SEC / SYS_HZ))); \
})

#define s_TO_jiffies(s)     ((double)SYS_HZ * (double)(s))
#define ms_TO_jiffies(ms)   (s_TO_jiffies(ms_TO_s(ms)))
#define us_TO_jiffies(us)   (s_TO_jiffies(us_TO_s(us)))
#define ns_TO_jiffies(ns)   (s_TO_jiffies(ns_TO_s(ns)))

#define jiffies_TO_s(j)     ((double)(j) / SYS_HZ)
#define jiffies_TO_ms(j)    (s_TO_ms(jiffies_TO_s(j)))
#define jiffies_TO_us(j)    (s_TO_us(jiffies_TO_s(j)))
#define jiffies_TO_ns(j)    (s_TO_ns(jiffies_TO_s(j)))

#define time_after(unknown, known)        ((long)(known) - (long)(unknown) < 0)
#define time_before(unknown, known)       ((long)(unknown) - (long)(known) < 0)
#define time_after_eq(unknown, known)     ((long)(unknown) - (long)(known) >= 0)
#define time_before_eq(unknown, known)    ((long)(known) - (long)(unknown) >= 0)

typedef unsigned long jiffies_t;

void jiffies_update(void);
void jiffies_timed_wait(double s);
jiffies_t jiffies_get(void);
jiffies_t jiffies_sleep(jiffies_t jiffies);

int jiffies_getres(struct timespec *res);
int jiffies_gettime(struct timespec *tp);
int jiffies_settime(const struct timespec *tp);