#pragma once

#define CLK_PIT     (0)
#define CLK_HPET    (1)
#define CLK_RTC     (2)
#define CLK_TSC     (3)
#define CLK_ANY     (-1)

void timer_intr(void);
void timer_wait(int tmr, double s);