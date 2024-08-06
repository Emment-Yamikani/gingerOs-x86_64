#pragma once

#include <types.h>

typedef struct rtc_time {
    uint8_t     sec;
    uint8_t     min;
    uint8_t     hrs;
    uint8_t     day;
    uint8_t     mon;
    uint16_t    year;
    uint8_t     cent;
} rtc_time_t;

#define RTC_GETTIME 0
#define RTC_SETTIME 1
#define RTC_SETALRM 2