#include <lib/stddef.h>
#include <lib/stdint.h>
#include <sys/system.h>
#include <boot/boot.h>
#include <lib/printk.h>
#include <arch/x86_64/system.h>
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
#include <ginger/jiffies.h>
#include <sys/sleep.h>
#include <sys/_signal.h>

__noreturn void kthread_main(void *arg __unused)
{
    int nthread = 0;

    printk("Welcome to 'Ginger OS'.\n");
    BUILTIN_THREAD_ANOUNCE(__func__);

    start_builtin_threads(&nthread, NULL);

    // don't allow main thread to exit.
    loop() {
        thread_info_t info = {0};

        if (!thread_join(0, &info, NULL))
            printk("thread[%d] exited status: %s cputime:%.1f\n", info.ti_tid,
                t_states[info.ti_state], jiffies_TO_s(info.ti_sched.cpu_time));
    }
}

void *garbbage_collector(void) {
    BUILTIN_THREAD_ANOUNCE(__func__);
    sleep(1);

    printk("thread[%d] done\n", thread_self());
    return 0;
}

BUILTIN_THREAD(garbbage_collector, garbbage_collector, NULL);