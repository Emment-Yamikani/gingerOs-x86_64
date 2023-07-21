#include <dev/dev.h>
#include <modules/module.h>
#include <bits/errno.h>
#include <lib/printk.h>
#include <mm/kalloc.h>

int rtc_init(void) {
    printk("Initializing Real Time Clock (RTC) timer...\n");
    return 0;
}




MODULE_INIT(RTC0, rtc_init, NULL);