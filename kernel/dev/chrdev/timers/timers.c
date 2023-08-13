#include <arch/chipset.h>
#include <bits/errno.h>
#include <dev/dev.h>
#include <dev/hpet.h>
#include <ginger/jiffies.h>
#include <sync/atomic.h>
#include <dev/clocks.h>

static atomic_t hpet_avl = {0};

int timer_init(void) {
    int err = 0;
    printk("Initializing timers...\n");
    if ((err = hpet_init()))
        pit_init(); // if failed to initalized HPET use PIT.
    else
        atomic_write(&hpet_avl, 1);
    printk("Timers initalized successfully.\n");
    return 0;
}

void timer_wait(int tmr, double s) {
    if (!s)
        return;
    switch (tmr) {
    case CLK_HPET:
        hpet_wait(s);
        break;
    case CLK_PIT:
        pit_wait(s);
        break;
    //case CLK_RTC:
    //case CLK_TSC:
    case CLK_ANY:
        __fallthrough;
    default:
        if (atomic_read(&hpet_avl))
            hpet_wait(s);
        else
            pit_wait(s);
    }
}

void timer_intr(void) {
    if (atomic_read(&hpet_avl))
        hpet_intr();
    else
        pit_intr();
}


MODULE_INIT(timer, timer_init, NULL);