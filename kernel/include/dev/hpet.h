#pragma once

#include <lib/stdint.h>
#include <lib/stddef.h>
#include <sync/spinlock.h>

typedef struct
{
    int t_id;          // timer ID.
    unsigned t_flags;  // timer operation flags.
    uint64_t t_value;  // initial counter value.
} hpet_timer_t;


#define HPET_TMR_32             BS(0)
#define HPET_TMR_PER            BS(1)
#define HPET_TMR_LVL            BS(2)

#define HPET_TMR_IS32(flags)    BTEST(flags, 0)
#define HPET_TMR_ISPER(flags)   BTEST(flags, 1)
#define HPET_TMR_ISLVL(flags)   BTEST(flags, 2)

int hpet_init(void);
void hpet_intr(void);
long hpet_freq(void);
void hpet_wait(double s);
size_t hpet_rdmcnt(void);
int hpet_timer_init(const hpet_timer_t *tmr);