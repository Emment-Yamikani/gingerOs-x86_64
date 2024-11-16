#pragma once

#include <lib/types.h>

/* A time value that is accurate to the nearest
   microsecond but also has a range of years.  */
struct timeval
{
#ifdef __USE_TIME64_REDIRECTS
    time64_t tv_sec;       /* Seconds.  */
    suseconds64_t tv_usec; /* Microseconds.  */
#else
    time_t tv_sec;       /* Seconds.  */
    suseconds_t tv_usec; /* Microseconds.  */
#endif
};
