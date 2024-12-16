#include <bits/errno.h>
#include <lib/string.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/thread.h>

int thread_join(tid_t tid, thread_info_t *info, void **retval) {
    int         err     = 0;
    thread_t    *thread = NULL;

    if (current_iskilled())
        return -EINTR;

    current_tgroup_lock();

    if ((err = tgroup_get_thread(current_tgroup(), tid, T_ZOMBIE, &thread))) {
        current_tgroup_unlock();
        return err;
    }

    current_tgroup_unlock();
    thread_assert_locked(thread);

    if ((err = thread_join_r(thread, info, retval))) {
        thread_unlock(thread);
        return err;
    }

    return 0;
}

int thread_join_r(thread_t *thread, thread_info_t *info, void **retval) {
    thread_assert_locked(thread);
    return thread_reap(thread, 1, info, retval);
}