#include <arch/cpu.h>
#include <bits/errno.h>
#include <lib/printk.h>
#include <lib/string.h>
#include <mm/kalloc.h>
#include <sys/thread.h>
#include <fs/fs.h>

void tgroup_destroy(tgroup_t *tgroup) {
    int err = 0;
    long ref = 0;

    if (tgroup == NULL)
        panic("No tgroup to destry.");
    
    if (current_tgroup() == tgroup)
        panic("Thread not allowed to "
            "destroy a tgroup it resides in.\n");

    if (!tgroup_islocked(tgroup))
        tgroup_lock(tgroup);

    if ((err = tgroup_kill_thread(tgroup, -1, 1)))
        panic("Failed to kill all threads, error: %d\n", err);

    tgroup_putref(tgroup);

    if ((ref = tgroup_getref(tgroup)) > 0)
        panic("Cannot destroy tgroup struct "
            "[in use by (%ld) other context(s)].\n", ref);
    
    if (tgroup_running(tgroup))
        panic("Still some threads running"
            " in [tgroup %d].\n", tgroup_getid(tgroup));

    tgroup_queue_lock(tgroup);
    queue_flush(tgroup_queue(tgroup));
    tgroup_queue_unlock(tgroup);

    assert(tgroup_getthread_count(tgroup) == 0, "No thread must be in tgroup.");

    tgroup_unlock(tgroup);
    kfree(tgroup);
}

int tgroup_kill_thread(tgroup_t *tgroup, tid_t tid, int wait) {
    int err = 0;
    thread_t *thread = NULL;
    queue_node_t *next = NULL;
    tgroup_assert_locked(tgroup);
    if (current_iskilled())
        return -EINTR;

    if (tid == -1) {
        tgroup_queue_lock(tgroup);
        forlinked(node, tgroup_queue(tgroup)->head, next) {
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

        /**
         * @brief 'current' becomes the main thread,
         * if it belongs to 'tgroup'.
         */
        if (current->t_group == tgroup)
            thread_setmain(current);
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
    tgroup_t    *tgroup = NULL;
    dentry_t    *cwd    = NULL;
    dentry_t    *rootdir= NULL;
    int         err     = -ENOMEM;

    if (ptgroup == NULL)
        return -EINVAL;

    if ((err = vfs_lookup("/", NULL, O_RDONLY, 0, 0, &cwd)))
        return err;

    rootdir = ddup(cwd);
    dunlock(cwd);

    if (NULL == (tgroup = (tgroup_t *)kmalloc(sizeof *tgroup)))
        goto error;

    memset(tgroup, 0, sizeof *tgroup);

    tgroup->tg_lock   = SPINLOCK_INIT();

    tgroup_lock(tgroup);
    tgroup->tg_refcnt = 1;
    tgroup->tg_thread = QUEUE_INIT();

    tgroup->tg_cred = CRED_DEFAULT();

    tgroup->tg_file_ctx.fc_cwd    = cwd;
    tgroup->tg_file_ctx.fc_root   = rootdir;
    tgroup->tg_file_ctx.fc_fmax   = NFILE;
    tgroup->tg_file_ctx.fc_lock   = SPINLOCK_INIT();

    *ptgroup = tgroup;
    return 0;
error:
    if (cwd)
        dclose(cwd);
    
    if (rootdir)
        dclose(rootdir);

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
    
    if ((err = thread_enqueue(tgroup_queue(tgroup), thread, NULL)))
        return err;
    
    thread->t_group         = tgroup;
    thread->t_credentials   = &tgroup->tg_cred;
    thread->t_file_ctx      = &tgroup->tg_file_ctx;

    if (tgroup_getthread_count(tgroup) == 1) {
        thread_setmain(thread);
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

    if ((err = thread_remove_queue(thread, tgroup_queue(tgroup)))) {
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

    forlinked(node, tgroup_queue(tgroup)->head, next) {
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

int tgroup_thread_create(tgroup_t *tgroup, thread_entry_t entry,
    void *arg, int flags, thread_t **pthread) {
    int err = 0;
    thread_t *thread = NULL;
    (void)entry, (void)arg, (void)flags, (void)pthread;
    
    if (!pthread)
        return -EINVAL;

    tgroup_assert_locked(tgroup);

    /// TODO: remove thread_new() and add appropriate call.
    return -ENOSYS;
    // if ((err = thread_new(NULL, entry, arg, flags, &thread)))
        // goto error;

    if ((err = tgroup_add_thread(tgroup, thread)))
        goto error;

    *pthread = thread;

    /**
     * @brief acknowledge schedule operation after creation.*/
    if (flags & THREAD_CREATE_SCHED)
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
    
    if ((err = tgroup_thread_create(tgroup, entry, arg, flags, &thread)))
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
    forlinked(node, tgroup_queue(tgroup)->head, next) {
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
        forlinked (node, tgroup_queue(tgroup)->head, next) {
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

int tgroup_sigprocmask(tgroup_t *tgroup, int how,
    const sigset_t *restrict set, sigset_t *restrict oset) {
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
    forlinked(node, tgroup_queue(tgroup)->head, next) {
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

int tgroup_getmain(tgroup_t *tgroup, thread_t **ptp) {
    thread_t *thread = NULL;

    if (tgroup == NULL || ptp == NULL)
        return -EINVAL;
    
    tgroup_assert_locked(tgroup);
    
    queue_lock(&tgroup->tg_thread);
    forlinked(node, tgroup->tg_thread.head, node->next) {
        thread = (thread_t *)node->data;
        thread_lock(thread);
        if (thread_ismain(thread)) {
            *ptp = thread_getref(thread);
            queue_unlock(&tgroup->tg_thread);
            return 0;
        }
        thread_unlock(thread);
    }
    queue_unlock(&tgroup->tg_thread);

    return -ESRCH;
}