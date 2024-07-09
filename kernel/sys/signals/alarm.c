#include <arch/cpu.h>
#include <arch/signal.h>
#include <arch/thread.h>
#include <bits/errno.h>
#include <dev/clocks.h>
#include <mm/kalloc.h>
#include <sys/proc.h>
#include <sys/_signal.h>
#include <sys/sysproc.h>
#include <sys/thread.h>
#include <ds/btree.h>

int raise(int signo) {
    return pthread_kill(thread_gettid(current), signo);
}

int pause(void) {
    sigset_t set        = 0;
    sigset_t empty_set  = 0;
    loop() {
        sigpending(&set);
        if (set != empty_set)
            break;
        thread_yield();
    }
    return -EINTR;
}

void trigger_alarm(pid_t pid) {
    kill(pid, SIGALRM);
}

unsigned long alarm(unsigned sec) {
    timeval_t       tv    = {0};
    clockid_t       clkid = 0;
    static btree_t alarms = {0};

    tv.tv_sec   = sec;
    tv.tv_usec  = 0;

    proc_lock(curproc);
    if (sec == 0) { // disarm previous alarm.
        btree_lock(&alarms);
        btree_search(&alarms, curproc->pid, (void **)&clkid);
        btree_unlock(&alarms);
        proc_unlock(curproc);
        return 0;
    }

    assert(
        clock_set(&tv,
        (void *)trigger_alarm,
        (void *)(long)curproc->pid,
        CLK_ARMED, &clkid
    ) == 0, "Failed to set alarm\n");
    
    btree_lock(&alarms);
    btree_insert(&alarms, curproc->pid, (void *)clkid);
    btree_unlock(&alarms);

    proc_unlock(curproc);

    return 0;
}