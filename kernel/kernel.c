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

void signal_handler(int signo) {
    printk("%s delivered: thread[%d]\n", signal_str[signo - 1], thread_self());
}

__noreturn void kthread_main(void) {
    sigset_t set;
    int nthread = 0;
    sigaction_t act = {0};

    BUILTIN_THREAD_ANOUNCE(__func__);
    printk("Welcome to 'Ginger OS'.\n");

    sigemptyset(&set);
    sigfillset(&set);
    sigdelset(&set, SIGKILL);
    sigdelset(&set, SIGSTOP);

    pthread_sigmask(SIG_SETMASK, &set, NULL);
    sigdelset(&set, SIGQUIT);
    sigprocmask(SIG_SETMASK, &set, NULL);
    
    act.sa_sigaction = NULL;
    sigfillset(&act.sa_mask);
    act.sa_handler = signal_handler;

    sigaction(SIGQUIT, &act, NULL);
    builtin_threads_begin(&nthread, NULL);

    current_tgroup_lock();
    tgroup_sigqueue(current_tgroup(), SIGQUIT);
    tgroup_sigqueue(current_tgroup(), SIGKILL);
    current_tgroup_unlock();

    loop() {
        thread_join(0, NULL, NULL);
    }
}