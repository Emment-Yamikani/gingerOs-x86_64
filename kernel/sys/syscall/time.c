#include <bits/errno.h>
#include <sys/_time.h>
#include <sys/system.h>
#include <dev/rtc.h>
#include <lib/types.h>
#include <dev/clocks.h>



int gettimeofday(struct timeval *restrict tp, void *restrict tzp __unused) {
    time_t time = rtc_gettime();
    tp->tv_usec = 0;
    tp->tv_sec  = time;
    return 0;
}