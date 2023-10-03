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
#include <modules/module.h>
#include <fs/tmpfs.h>

void core_start(void);

void signal_handler(int signo) {
    printk("%s delivered: thread[%d]\n", signal_str[signo - 1], thread_self());
}

EXPORT_SYMBOL(signal_handler);

__noreturn void kthread_main(void) {
    int nthread = 0;
    printk("Welcome to 'Ginger OS'.\n");
    
    builtin_threads_begin(&nthread, NULL);

    loop() thread_join(0, NULL, NULL);
}