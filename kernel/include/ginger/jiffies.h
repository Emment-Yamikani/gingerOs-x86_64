#pragma once

#include <lib/stdint.h>
#include <lib/stddef.h>
#include <sys/_time.h>

// The system heartbeat @ 100Hz = 10000000ns
#define SYS_HZ (100)

#define HZ_TO_s(Hz)         ((double)1 / (Hz))          // convert Hz to s
#define HZ_TO_ms(Hz)        ((double)1000 / (Hz))       // convert Hz to ms
#define HZ_TO_us(Hz)        ((double)1000000 / (Hz))    // convert Hz to us
#define HZ_TO_ns(Hz)        ((double)1000000000 / (Hz)) // convert Hz to ns

#define seconds_TO_ns(s)    ((s)*1000000000)    // convert seconds to ns
#define seconds_TO_us(s)    ((s)*1000000)       // convert seconds to us
#define seconds_TO_ms(s)    ((s)*1000)          // convert seconds to ms

#define jiffies_TO_s(jiffy) (jiffy * ((double)HZ_TO_ns(SYS_HZ) / seconds_TO_ns(1)))

#define time_after(unknown, known)        ((long)(known) - (long)(unknown) < 0)
#define time_before(unknown, known)       ((long)(unknown) - (long)(known) < 0)
#define time_after_eq(unknown, known)     ((long)(unknown) - (long)(known) >= 0)
#define time_before_eq(unknown, known)    ((long)(known) - (long)(unknown) >= 0)

typedef unsigned long jiffies_t;

void jiffies_update(void);
jiffies_t jiffies_get(void);
jiffies_t jiffies_sleep(jiffies_t jiffies);

int jiffies_getres(struct timespec *res);
int jiffies_gettime(struct timespec *tp);
int jiffies_settime(const struct timespec *tp);