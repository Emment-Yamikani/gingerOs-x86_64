#include <bits/errno.h>
#include <sys/_time.h>
#include <sys/system.h>

int gettimeofday(struct timeval *restrict tp __unused, void *restrict tzp __unused) {
    return -ENOSYS;
}