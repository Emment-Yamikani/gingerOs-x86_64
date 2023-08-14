#include <arch/x86_64/system.h>
#include <lib/stddef.h>
#include <lib/stdint.h>
#include <sys/system.h>
#include <boot/boot.h>
#include <ginger/jiffies.h>
#include <lib/printk.h>
#include <mm/pmm.h>
#include <mm/mm_zone.h>
#include <mm/vmm.h>
#include <mm/kalloc.h>
#include <arch/paging.h>
#include <ds/queue.h>
#include <sys/thread.h>
#include <sync/cond.h>
#include <arch/lapic.h>
#include <arch/traps.h>
#include <dev/dev.h>
#include <sys/sleep.h>
#include <sys/_signal.h>
#include <dev/hpet.h>

void core_start(void);

__noreturn void kthread_main(void) {
    int nthread = 0;
    BUILTIN_THREAD_ANOUNCE(__func__);
    printk("Welcome to 'Ginger OS'.\n");

    builtin_threads_begin(&nthread, NULL);
    
    core_start();

    loop() {
        thread_join(0, NULL, NULL);
    }
}

void *garbbage_collector(void) {
    BUILTIN_THREAD_ANOUNCE(__func__);
    jiffies_timed_wait(1);
    return 0;
}

BUILTIN_THREAD(garbbage_collector, garbbage_collector, NULL);