#include <sys/sleep.h>
#include <sys/thread.h>
#include <lib/printk.h>
#include <ginger/jiffies.h>
#include <sys/sched.h>
#include <ds/queue.h>
#include <bits/errno.h>

queue_t *global_sleep_queue = QUEUE_NEW("Global sleep queue");

long sleep(long s) {
    jiffies_t jiffies = s_TO_ns(s) / HZ_TO_ns(SYS_HZ);

    jiffies = jiffies_sleep(jiffies);
    if (jiffies <= 0)
        return 0;
    s = jiffies_TO_s(jiffies);
    return s;
}

int park(void) {
    int err = 0;
    current_lock();

    if (current_testflags(THREAD_SETWAKE)) {
        current_maskflags(THREAD_SETPARK | THREAD_SETWAKE);
        return 0;
    }

    err = sched_sleep(global_sleep_queue, NULL);

    current_unlock();
    return err;
}

int unpark(tid_t tid) {
    int err = 0;
    tgroup_t *tgrp = NULL;
    thread_t *thread = NULL;

    queue_lock(global_sleep_queue);

    if ((err = thread_queue_get(global_sleep_queue, tid, &thread))) {
        if (err != -ESRCH)
            goto error;

        current_lock();
        tgrp = current->t_group;
        current_unlock();

        tgroup_lock(tgrp);
        if ((err = thread_get(tgrp, tid, &thread))) {
            tgroup_unlock(tgrp);
            goto error;
        }
        tgroup_unlock(tgrp);
    }

    if (thread == current) {
        err = -EINVAL;
        current_unlock();
        goto error;
    }

    if ((err = thread_wake(thread))) {
        thread_unlock(thread);
        goto error;
    }

    thread_unlock(thread);
    queue_unlock(global_sleep_queue);
    return 0;
error:
    queue_unlock(global_sleep_queue);
    return err;
}

void setpark(void) {
    current_lock();
    current_setflags(THREAD_SETPARK);
    current_unlock();
}