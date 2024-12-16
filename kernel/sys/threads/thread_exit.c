#include <bits/errno.h>
#include <lib/string.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/thread.h>

int thread_reap(thread_t *thread, int reap, thread_info_t *info, void **retval) {
    int err = 0;
    thread_assert_locked(thread);
    if (current == thread)
        return -EDEADLK;

    while(!thread_iszombie(thread)) {
        if (current_iskilled())
            return -EINTR;
        thread_unlock(thread);
        err = cond_wait(&thread->t_wait);
        thread_lock(thread);
        if (err) return err;
    }

    if (info) {
        info->ti_tid    = thread->t_tid;
        info->ti_exit   = thread->t_exit;
        info->ti_ktid   = thread->t_ktid;
        info->ti_flags  = thread->t_flags;
        info->ti_errno  = thread->t_errno;
        info->ti_state  = thread->t_state;
        info->ti_sched  = thread->t_sched;
    }

    if (retval)
        *retval = (void *)thread->t_exit;

    if(reap)
        thread_free(thread);
    return 0;
}

int thread_kill_n(thread_t *thread, int wait) {
    int err = 0;
    thread_assert_locked(thread);

    if (thread_iszombie(thread)||
        thread_isterminated(thread))
        return 0;
    
    thread_setflags(thread, THREAD_KILLED);
    
    if ((err = thread_wake(thread)))
        return err;

    return thread_reap(thread, wait, NULL, NULL);
}

int thread_kill(tid_t tid, int wait) {
    int     err     = 0;
    queue_t *tgroup = NULL;

    if (tid < 0)
        return -EINVAL;

    current_lock();
    tgroup = current_tgroup();
    current_unlock();

    tgroup_lock(tgroup);
    err = tgroup_kill_thread(tgroup, tid, 0, wait);
    tgroup_unlock(tgroup);

    return err;
}

int thread_kill_all(void) {
    int err = 0;
    current_assert();
    tgroup_lock(current_tgroup());
    err = tgroup_kill_thread(current_tgroup(), -1, 0, 1);
    tgroup_unlock(current_tgroup());
    return err;
}

void thread_exit(uintptr_t exit_code) {
    current_assert();
    arch_thread_exit(exit_code);
}