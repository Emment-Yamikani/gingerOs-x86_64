#include <sys/sleep.h>
#include <sys/thread.h>
#include <lib/printk.h>
#include <ginger/jiffies.h>
#include <sys/sched.h>
#include <ds/queue.h>
#include <bits/errno.h>

static queue_t *global_sleep_queue = QUEUE_NEW("Global sleep queue");

long sleep(long s) {
    jiffies_t jiffies = s_TO_jiffies(s);

    jiffies = jiffies_sleep(jiffies);
    if (jiffies <= 0)
        return 0;
    s = jiffies_TO_s(jiffies);
    return s;
}

int park(void) {
    int err = 0;
    current_lock();

    current_setpark();

    if (current_issetwake()) {
        current_mask_park_wake();
        current_unlock();
        return 0;
    }

    err = sched_sleep(global_sleep_queue, NULL);

    current_mask_park_wake();
    current_unlock();
    return err;
}

int unpark(tid_t tid) {
    int err = 0;
    thread_t *thread = NULL;

    current_tgroup_lock();
    if ((err = tgroup_get_thread(current_tgroup(), tid, T_ISLEEP, &thread))) {
        current_tgroup_lock();
        return err;
    }
    current_tgroup_unlock();

    thread_setwake(thread);
    err = thread_wake(thread);
    thread_unlock(thread); 
    return err;
}