#include <arch/cpu.h>
#include <bits/errno.h>
#include <lib/printk.h>
#include <lib/string.h>
#include <mm/kalloc.h>
#include <sys/thread.h>

int tgroup_destroy(tgroup_t *tgroup) {
    int err = 0;
    // __unused thread_t *thread = NULL;

    if ((tgroup == NULL) || (current_tgroup() == tgroup))
        return -EINVAL;

    tgroup_assert_locked(tgroup);

    if ((err = tgroup_kill_thread(tgroup, -1, 1))) {
        tgroup_unlock(tgroup);
        goto error;
    }

    assert(!tgroup_thread_count(tgroup), "There is still some thread(s).");

    queue_free(tgroup->tg_queue);
    queue_free(tgroup->tg_stopq);

    tgroup_unlock(tgroup);

    kfree(tgroup);

    return 0;
error:
    return err;
}

size_t tgroup_inc_running(tgroup_t *tgroup) {
    tgroup_assert_locked(tgroup);
    return ++tgroup->tg_running;
}

size_t tgroup_dec_running(tgroup_t *tgroup) {
    tgroup_assert_locked(tgroup);
    return --tgroup->tg_running;
}

size_t tgroup_thread_count(tgroup_t *tgroup) {
    size_t count = 0;
    tgroup_queue_lock(tgroup);
    count = queue_count(tgroup->tg_queue);
    tgroup_queue_unlock(tgroup);
    return count;
}

size_t tgroup_running_threads(tgroup_t *tgroup) {
    tgroup_assert_locked(tgroup);
    return tgroup->tg_running;
}

int tgroup_kill_thread(tgroup_t *tgroup, tid_t tid, int wait) {
    int err = 0;
    thread_t *thread = NULL;
    queue_node_t *next = NULL;
    tgroup_assert_locked(tgroup);
    if (current_killed())
        return -EINTR;

    if (tid == -1) {
        tgroup_queue_lock(tgroup);
        forlinked(node, tgroup->tg_queue->head, next) {
            next = node->next;
            thread = node->data;
            if (thread == current)
                continue;

            thread_lock(thread);
            tgroup_queue_unlock(tgroup);
            tgroup_unlock(tgroup);

            if ((err = thread_kill_n(thread, wait))) {
                thread_unlock(thread);
                goto error;
            }

            if (wait == 0)
                thread_unlock(thread);
            tgroup_lock(tgroup);
            tgroup_queue_lock(tgroup);
        }
        tgroup_queue_unlock(tgroup);
    } else {
        if ((err = tgroup_get_thread(tgroup, tid, 0, &thread)))
            goto error;

        thread_assert_locked(thread);

        if (thread == current) {
            current_unlock();
            thread_exit(0);
        }

        if ((err = thread_kill_n(thread, wait))) {
            thread_unlock(thread);
            goto error;
        }

        thread_unlock(thread);
    }

    return 0;
error:
    return err;
}

int tgroup_create(tgroup_t **ptgroup) {
    int err = 0;
    tgroup_t *tgroup = NULL;
    queue_t *stopq = NULL, *queue = NULL;

    if (!ptgroup)
        return -EINVAL;
    
    if (!(tgroup = kmalloc(sizeof *tgroup)))
        return -ENOMEM;


    if ((err = queue_new("tgroup-stop", &stopq)))
        goto error;
    
    if ((err = queue_new("tgroup", &queue)))
        goto error;

    memset(tgroup, 0, sizeof *tgroup);

    tgroup->tg_queue = queue;
    tgroup->tg_stopq = stopq;
    tgroup->tg_lock = SPINLOCK_INIT();

    tgroup_lock(tgroup);
    *ptgroup = tgroup;

    return 0;
error:
    if (queue)
        queue_free(queue);
    if (stopq)
        queue_free(stopq);
    if (tgroup)
        kfree(tgroup);
    return err;
}

int tgroup_add_thread(tgroup_t *tgroup, thread_t *thread) {
    int err = 0;
    tgroup_assert_locked(tgroup);
    thread_assert_locked(thread);

    if (!thread)
        return -EINVAL;
    
    if ((err = thread_enqueue(tgroup->tg_queue, thread, NULL)))
        return err;
    
    thread->t_group = tgroup;
    if (tgroup->tg_tmain == NULL) {
        tgroup->tg_tmain = thread;
        tgroup->tg_tlast = thread;
        tgroup->tg_tgid = thread->t_tid;
    }
    
    return 0;
}

int tgroup_remove_thread(tgroup_t *tgroup, thread_t *thread) {
    int err = 0;
    tgroup_assert_locked(tgroup);

    if (!thread)
        return -EINVAL;

    tgroup_queue_lock(tgroup);
    thread_lock(thread);

    if ((err = thread_remove_queue(thread, tgroup->tg_queue))) {
        thread_unlock(thread);
        tgroup_queue_unlock(tgroup);
        return err;
    }

    thread->t_group = NULL;
    thread_unlock(thread);
    tgroup_queue_unlock(tgroup);

    return 0;
}

int tgroup_get_thread(tgroup_t *tgroup, tid_t tid, tstate_t state, thread_t **pthread) {
    thread_t *thread = NULL;
    queue_node_t *next = NULL;

    if (!pthread)
        return -EINVAL;

    tgroup_queue_lock(tgroup);

    forlinked(node, tgroup->tg_queue->head, next) {
        next = node->next;
        thread = node->data;

        thread_lock(thread);

        if (tid == 0 && thread_isstate(thread, state)) {
            tgroup_queue_unlock(tgroup);
            *pthread = thread;
            return 0;
        }

        if (thread->t_tid == tid || tid == -1) {
            tgroup_queue_unlock(tgroup);
            *pthread = thread;
            return 0;
        }

        thread_unlock(thread);
    }

    tgroup_queue_unlock(tgroup);

    return -ESRCH;
}

int tgroup_thread_create(tgroup_t *tgroup, thread_entry_t entry, void *arg, int flags, int sched, thread_t **pthread) {
    int err = 0;
    thread_t *thread = NULL;
    
    if (!pthread)
        return -EINVAL;

    tgroup_assert_locked(tgroup);
    if ((err = thread_new(NULL, entry, arg, flags, &thread)))
        goto error;

    if ((tgroup_add_thread(tgroup, thread)))
        goto error;

    *pthread = thread;

    if (sched)
        thread_schedule(thread);
    
    return 0;
error:
    if (thread) thread_free(thread);
    return err;
}

int tgroup_spawn(thread_entry_t entry, void *arg, int flags, tgroup_t **ptgroup) {
    int err = 0;
    tgroup_t *tgroup = NULL;
    thread_t *thread = NULL;

    if ((err = tgroup_create(&tgroup)))
        return err;
    
    if ((err = tgroup_thread_create(tgroup, entry, arg, flags, 0, &thread)))
        goto error;
    
    *ptgroup = tgroup;
    thread_unlock(thread);

    return 0;
error:
    if (thread) thread_free(thread);
    if (tgroup) tgroup_destroy(tgroup);
    return err;
}

int tgroup_terminate(tgroup_t *tgroup, spinlock_t *lock) {
    int err = 0;
    tgroup_assert_locked(tgroup);
    if (lock)
        spin_unlock(lock);

    if ((err = tgroup_kill_thread(tgroup, -1, 0)))
        printk("ERROR OCCURED: %d\n", err);
    if (current_tgroup() == tgroup)
    {
        tgroup_unlock(tgroup);
        thread_exit(-EINTR);
    }
    return err;
}

int tgroup_continue(tgroup_t *tgroup) {
    thread_t *thread = NULL;
    queue_node_t *next = NULL;

    tgroup_assert_locked(tgroup);

    tgroup_queue_lock(tgroup);
    forlinked(node, tgroup->tg_queue->head, next) {
        next = node->next;
        thread = node->data;
        thread_lock(thread);
        thread_wake(thread);
        thread_unlock(thread);
    }
    tgroup_queue_unlock(tgroup);

    return 0;
}

int tgroup_sigqueue(tgroup_t *tgroup, int signo) {
    thread_t *thread = NULL;
    queue_node_t *next = NULL;
    int err = 0, __unused send_current = 0;

    if (SIGBAD(signo))
        return -EINVAL;

    tgroup_assert_locked(tgroup);

    switch (sigismember(&tgroup->sig_mask, signo)) {
    case 0:
        tgroup->sig_queues[signo - 1]++;
        tgroup_queue_lock(tgroup);
        forlinked (node, tgroup->tg_queue->head, next) {
            err = -EINVAL;
            next = node->next;
            thread = node->data;
  
            thread_lock(thread);
            if (thread_isstopped(thread) &&
                ((signo != SIGKILL) &&
                 (signo != SIGCONT))) {
                thread_unlock(thread);
                continue;
            }

            if (!thread_isterminated(thread) &&
                !thread_iszombie(thread)) {
                if ((err = sigismember(&thread->t_sigmask, signo)) == 0)
                    err = thread_wake(thread);
            }
            thread_unlock(thread);

            if (err == 0)
                break;
        }
        tgroup_queue_unlock(tgroup);
        break;
    case -EINVAL:
    case 1:
        err = -EINVAL;
        break;
    }

    return err;
}

int tgroup_sigprocmask(tgroup_t *tgroup, int how, const sigset_t *restrict set, sigset_t *restrict oset) {
    int err = 0;
    tgroup_assert_locked(tgroup);

    if (oset)
        *oset = tgroup->sig_mask;

    if (set == NULL)
        return 0;

    if (sigismember(set, SIGKILL) || sigismember(set, SIGSTOP))
        return -EINVAL;

    switch (how) {
    case SIG_BLOCK:
        tgroup->sig_mask |= *set;
        break;
    case SIG_UNBLOCK:
        tgroup->sig_mask &= ~*set;
        break;
    case SIG_SETMASK:
        tgroup->sig_mask = *set;
        break;
    default:
        err = -EINVAL;
        break;
    }

    return err;
}

int tgroup_stop(tgroup_t *tgroup) {
    thread_t *thread = NULL;
    queue_node_t *next = NULL;
    tgroup_assert_locked(tgroup);

    tgroup_queue_lock(tgroup);
    forlinked(node, tgroup->tg_queue->head, next) {
        next = node->next;
        thread = node->data;
        thread_lock(thread);
        thread_setflags(thread, THREAD_STOP);
        thread_wake(thread);
        thread_unlock(thread);
    }
    tgroup_queue_unlock(tgroup);
    return 0;
}