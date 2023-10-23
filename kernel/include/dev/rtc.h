#pragma once

#define RTC_GETTIME  0
#define RTC_SETTIME  1
#define RTC_SETALM   2

typedef struct rtc_time {
    uint8_t rtc_sec;
    uint8_t rtc_min;
    uint8_t rtc_hrs;
    uint8_t rtc_day;
    uint8_t rtc_mon;
    uint16_t rtc_year;
    uint8_t rtc_cent;
} rtc_time_t;

void rtc_intr(void);