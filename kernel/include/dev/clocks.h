#pragma once

#include <sync/spinlock.h>
#include <sys/system.h>
#include <sys/_time.h>

#define CLK_PIT     (0)
#define CLK_HPET    (1)
#define CLK_RTC     (2)
#define CLK_TSC     (3)
#define CLK_ANY     (-1)

typedef struct __clk_t {
    struct timeval  clk_tv;
    struct timeval  clk_default_tv;
    int             clk_flags;
    long            clk_id;
    void            *clk_arg;
    spinlock_t      clk_spinlock;
    void            (*clk_entry)(); 
} clk_t;

#define CLK_ARMED   BS(0)
#define CLK_RESET   BS(1)

#define NCLK    (4096)

#define clk_assert(clk)         ({ assert(clk, "No clock ptr"); })
#define clk_lock(clk)           ({ clk_assert(clk); spin_lock(&(clk)->clk_spinlock); })
#define clk_unlock(clk)         ({ clk_assert(clk); spin_unlock(&(clk)->clk_spinlock); })
#define clk_islocked(clk)       ({ clk_assert(clk); spin_islocked(&(clk)->clk_spinlock); })
#define clk_assert_locked(clk)  ({ clk_assert(clk); spin_assert_locked(&(clk)->clk_spinlock); })

int clock_set(timeval_t *ts, void (*entry)(), void *arg, int flags, clockid_t *ref);

void timer_intr(void);
void timer_wait(int tmr, double s);
void clock_trigger(void);