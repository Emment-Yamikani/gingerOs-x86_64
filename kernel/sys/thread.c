#include <lib/stdint.h>
#include <sys/thread.h>
#include <sys/sched.h>
#include <bits/errno.h>
#include <mm/kalloc.h>
#include <mm/vmm.h>
#include <sys/system.h>
#include <lib/string.h>
#include <ginger/jiffies.h>
#include <arch/thread.h>
#include <sys/proc.h>

const char *t_states[] = {
    [T_EMBRYO]      = "EMBRYO",
    [T_READY]       = "READY",
    [T_RUNNING]     = "RUNNING",
    [T_ISLEEP]      = "ISLEEP",
    [T_USLEEP]      = "USLEEP",
    [T_STOPPED]     = "STOPPED",
    [T_TERMINATED]  = "TERMINATED",
    [T_ZOMBIE]      = "ZOMBIE",
};

// convert from tstate_t literal to char *
char *tget_state(const tstate_t st) {
    return (char *)t_states[st];
}

static tid_t tid_alloc(void) {
    /**
     * Used Thread ID allocation.
     * to implement a way in which we can reuse tids.
     * use a tree maybe.
     * recycle a tid if and only if the thread group
     * in which rid was has terminated completely.
     */
    static atomic_t tids = {0};
    return atomic_inc_fetch(&tids);
}

uintptr_t thread_alloc_kstack(size_t size) {
    return (uintptr_t)kmalloc(size);
}

void thread_free_kstack(uintptr_t addr, size_t size __unused) {
    kfree((void *)addr);
}

int thread_alloc(uintptr_t kstacksz, int flags, thread_t **ref) {
    int         err         = 0;
    uintptr_t   kstack      = 0;
    thread_t    *thread     = NULL;

    if (ref == NULL)
        return -EINVAL;
    
    if (BADSTACKSZ(kstacksz))
        return -EINVAL;

    if ((kstack = thread_alloc_kstack(kstacksz)) == 0)
        return -ENOMEM;
    
    thread = (thread_t *)ALIGN16((kstack + kstacksz) - sizeof *thread);

    memset(thread, 0, sizeof *thread);

    if ((err = queue_alloc(&thread->t_queues)))
        goto error;

    thread->t_lock  = SPINLOCK_INIT();
    thread_lock(thread);
    
    thread_enter_state(thread, T_EMBRYO);
    thread_setflags(thread, THREAD_DETACHED);
    thread_setflags(thread, BTEST(flags, 0) ? THREAD_USER : 0);

    thread->t_arch  = (arch_thread_t){
        .t_thread   = thread,
        .t_kstack   = kstack,
        .t_kstacksz = kstacksz,
    };

    thread->t_refcnt                    = 1;
    thread->t_sched.ts_cpu_affinity_set = -1;
    thread->t_tid                       = tid_alloc();
    thread->t_wait                      = COND_INIT();
    thread->t_sched.ts_ctime            = jiffies_get();
    thread->t_sched.ts_affinity_type    = SCHED_SOFT_AFFINITY;
    thread->t_sched.ts_priority         = SCHED_LOWEST_PRIORITY;

    *ref = thread;
    return 0;
error:
    if (kstack) thread_free_kstack(kstack, KSTACKSZ);
    return err;
}

int kthread_create(thread_attr_t *attr, thread_entry_t entry, void *arg, thread_t **ptp) {
    int err = 0;
    thread_t *thread = NULL;
    tgroup_t *tgroup = NULL;

    thread_attr_t t_attr =
    attr ? *attr : (thread_attr_t) {
        .detachstate    = 0,
        .guardsz        = 0,
        .stackaddr      = 0,
        .stacksz        = USTACKSZ,
    };

    t_attr.stackaddr    = 0;
    if ((err = thread_alloc(t_attr.stacksz, 0, &thread)))
        return err;
    
    if ((err = arch_kthread_init(&thread->t_arch, entry, arg)))
        goto error;

    if (current_tgroup()) {
        current_tgroup_lock();
        if ((err = tgroup_add_thread(current_tgroup(), thread))) {
            current_tgroup_unlock();
            goto error;
        }
        current_tgroup_unlock();
    } else {
        if ((err = tgroup_create(&tgroup)))
            goto error;

        if ((err = tgroup_add_thread(tgroup, thread))) {
            tgroup_unlock(tgroup);
            goto error;
        }
        tgroup_unlock(tgroup);
    }

    if ((err = thread_schedule(thread)))
        goto error;

    if (ptp)
        *ptp = thread;
    else
        thread_unlock(thread);

    return 0;
error:
    if (tgroup)
        tgroup_destroy(tgroup);
    if (thread)
        thread_free(thread);
    return err;
}

int thread_create(thread_attr_t *attr, thread_entry_t entry, void *arg, thread_t **pthread) {
    int err = 0;
    vmr_t *ustack = NULL;
    thread_t *thread = NULL;
    thread_attr_t   t_attr = {0};
    tgroup_t *tgroup = current_tgroup();
    
    t_attr = attr ? *attr : (thread_attr_t){
        .detachstate    = 0,
        .stackaddr      = 0,
        .guardsz        = PGSZ,
        .stacksz        = USTACKSZ,
    };


    if (tgroup == NULL || curproc == NULL)
        return -EINVAL;

    if ((err = thread_alloc(KSTACKSZ, THREAD_USER, &thread)))
        return err;

    proc_lock(curproc);
    proc_mmap_lock(curproc);

    if (t_attr.stackaddr == 0) {
        if ((err = mmap_alloc_stack(proc_mmap(curproc), t_attr.stacksz, &ustack))) {
            proc_mmap_unlock(curproc);
            proc_unlock(curproc);
            goto error;
        }
    } else {
        err = -EINVAL;
        if (NULL == (ustack = mmap_find(proc_mmap(curproc), t_attr.stackaddr))) {
            proc_mmap_unlock(curproc);
            proc_unlock(curproc);
            goto error;
        }

        if (__isstack(ustack) == 0) {
            proc_mmap_unlock(curproc);
            proc_unlock(curproc);
            goto error;
        }
    }

    thread->t_mmap = proc_mmap(curproc);
    proc_mmap_unlock(curproc);
    proc_unlock(curproc);

    /// TODO: Optmize size of ustack according to attr;
    /// TODO: maybe perform a split? But then this will mean
    /// free() and unmap() calls to reverse malloc() and mmap() respectively
    /// for this region may fail. ???
    thread->t_arch.t_ustack = ustack;
    thread->t_owner = curproc;

    if ((err = arch_uthread_init(&thread->t_arch, entry, arg)))
        goto error;

    tgroup_lock(tgroup);
    if ((err = tgroup_add_thread(tgroup, thread))) {
        tgroup_unlock(tgroup);
        goto error;
    }
    tgroup_unlock(tgroup);

    if (pthread)
        *pthread = thread;
    else
        thread_unlock(thread);
    return 0;
error:
    if (thread) thread_free(thread);
    return err;
}

int thread_alloc_stack(thread_t *thread, thread_attr_t *attr, mmap_t *mmap) {
    int             err = 0;
    vmr_t           *ustack = NULL;
    thread_attr_t   t_attr = {0};

    if (thread == NULL || mmap == NULL)
        return -EINVAL;

    t_attr = attr ? *attr : (thread_attr_t){
        .detachstate    = 0,
        .guardsz        = 0,
        .stackaddr      = 0,
        .stacksz        = USTACKSZ,
    };

    mmap_assert_locked(mmap);

    if (t_attr.stackaddr == 0) {
        if ((err = mmap_alloc_stack(mmap, t_attr.stacksz, &ustack)))
            goto error;
    } else {
        err = -EINVAL;
        if (NULL == (ustack = mmap_find(mmap, t_attr.stackaddr)))
            goto error;

        if (__isstack(ustack) == 0)
            goto error;
    }

    /// TODO: Optmize size of ustack according to attr;
    /// TODO: maybe perform a split? But then this will mean
    /// free() and unmap() calls to reverse malloc() and mmap() respectively
    /// for this region may fail. ???
    thread->t_arch.t_ustack = ustack;

    return 0;
error:
    return err;
}

void thread_free(thread_t *thread) {
    queue_t *queue = NULL;
    queue_node_t *next = NULL;

    assert (current != thread, "current called freeing it's own kstack???");

    thread_assert_locked(thread);
    assert(thread_iszombie(thread), "freeing a non zombie thread");

    /**
     * Get rid of all the queue with qhich this thread is associated.
    */
    if (thread->t_queues) {
        queue_lock(thread->t_queues);
        forlinked(node, thread->t_queues->head, next) {
            next = node->next;
            queue = (queue_t *)node->data;
            // queue_unlock(thread->t_queues);
            queue_lock(queue);
            // queue_lock(thread->t_queues);
            if (queue_remove(queue, thread))
                panic("failed to remove thread from queue\n");

            if (queue_remove(thread->t_queues, queue))
                panic("failed to remove queue from thread-queue\n");

            queue_unlock(queue);
        }
        queue_unlock(thread->t_queues);
        queue_free(thread->t_queues);
    }

    assert(thread->t_arch.t_kstack, "??? No kernel stack ???");

    thread_unlock(thread);
    thread_free_kstack(thread->t_arch.t_kstack, thread->t_arch.t_kstacksz);
}

int thread_detach(thread_t *thread) {
    int err = 0;
    tgroup_t *tgroup = NULL;

    thread_tgroup_lock(thread);
    err = tgroup_remove_thread((tgroup = thread_tgroup(thread)), thread);
    tgroup_unlock(tgroup);
    return err;
}

tid_t thread_gettid(thread_t *thread) {
    return thread ? thread->t_tid : 0;
}

tid_t thread_self(void) {
    return thread_gettid(current);
}

void thread_exit(uintptr_t exit_code) {
    current_assert();
    arch_thread_exit(exit_code);
}

void thread_yield(void) {
    sched_yield();
}

int thread_enqueue(queue_t *queue, thread_t *thread, queue_node_t **rnode) {
    int err = 0;
    queue_node_t *node = NULL;
    
    if (!thread)
        return -EINVAL;

    queue_lock(queue);
    queue_lock(thread->t_queues);

    if ((err = enqueue(queue, (void *)thread, 1, NULL))) {
        queue_unlock(thread->t_queues);
        queue_unlock(queue);
        goto error;
    }

    if ((err = enqueue(thread->t_queues, (void *)queue, 1, NULL))) {
        node = NULL;
        queue_remove(queue, (void *)thread);
        queue_unlock(thread->t_queues);
        queue_unlock(queue);
        goto error;
    }

    queue_unlock(thread->t_queues);
    queue_unlock(queue);

    if (rnode)
        *rnode = node;

    return 0;
error:
    printk("fialed to enqueue thread, error: %d\n", err);
    return err;
}

thread_t *thread_dequeue(queue_t *queue) {
    int err = 0;
    thread_t *thread = NULL;
    assert(queue, "no queue");
    queue_assert_locked(queue);

    if ((err = dequeue(queue, (void **)&thread)))
        return NULL;
    thread_lock(thread);
    queue_lock(thread->t_queues);
    if ((err = queue_remove(thread->t_queues, (void *)queue)))
        panic("queue is not on thread[%d]'s threads-queue, error: %d\n", thread_gettid(thread), err);
    queue_unlock(thread->t_queues);
    return thread;
}

int thread_remove_queue(thread_t *thread, queue_t *queue) {
    int err = 0;
    queue_assert_locked(queue);
    thread_assert_locked(thread);

    queue_lock(thread->t_queues);
    if ((err = queue_remove(thread->t_queues, (void *)queue))) {
        queue_unlock(thread->t_queues);
        return err;
    }
    queue_unlock(thread->t_queues);
    return queue_remove(queue, (void *)thread);
}

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
    int err = 0;
    tgroup_t *tgroup = NULL;

    if (tid < 0)
        return -EINVAL;

    current_lock();
    tgroup = current_tgroup();
    current_unlock();

    tgroup_lock(tgroup);
    err = tgroup_kill_thread(tgroup, tid, wait);
    tgroup_unlock(tgroup);

    return err;
}

int thread_join(tid_t tid, thread_info_t *info, void **retval) {
    int err = 0;
    thread_t *thread = NULL;

    current_assert();

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

int thread_kill_all(void) {
    int err = 0;
    current_assert();
    tgroup_lock(current_tgroup());
    err = tgroup_kill_thread(current_tgroup(), -1, 1);
    tgroup_unlock(current_tgroup());
    return err;
}

int thread_wake(thread_t *thread) {
    int err = 0, guard_locked = 0, q_locked = 0;
    thread_assert_locked(thread);

    if (current == thread)
    return 0;

    if (thread_iszombie(thread) ||
        thread_isterminated(thread))
        return 0;

    if (thread_issetpark(thread))
        thread_setwake(thread);

    if (thread->sleep_attr.queue == NULL)
        return 0;

    if (thread->sleep_attr.guard && (guard_locked = !spin_islocked(thread->sleep_attr.guard)))
        spin_lock(thread->sleep_attr.guard);

    if ((q_locked = !queue_islocked(thread->sleep_attr.queue)))
        queue_lock(thread->sleep_attr.queue);

    err = thread_remove_queue(thread, thread->sleep_attr.queue);

    if (q_locked)
        queue_unlock(thread->sleep_attr.queue);

    if (thread->sleep_attr.guard && guard_locked)
        spin_unlock(thread->sleep_attr.guard);

    if (thread_isstopped(thread))
        thread_maskflags(thread, THREAD_STOP);

    if (err) return err;

    if ((err = thread_enter_state(thread, T_READY)))
        return err;

    return thread_schedule(thread);
}

int thread_stop(thread_t *thread, queue_t *queue) {
    thread_assert_locked(thread);
    return sched_sleep(queue, T_STOPPED, NULL);
}

int thread_queue_get(queue_t *queue, tid_t tid, thread_t **pthread) {
    thread_t *thread = NULL;
    queue_node_t *next = NULL;

    if (!queue || !pthread)
        return -EINVAL;

    queue_assert_locked(queue);
    forlinked(node, queue->head, next) {
        next = node->next;
        thread = node->data;

        thread_lock(thread);
        if (tid == 0) {
            *pthread = thread;
            return 0;
        } else if (thread->t_tid == tid)
        {
            *pthread = thread;
            return 0;
        }
        thread_unlock(thread);
    }

    return -ESRCH;
}

int thread_sigqueue(thread_t *thread, int signo) {
    int err = 0;

    if (!thread || SIGBAD(signo))
        return -EINVAL;
    thread_assert_locked(thread);

    if (thread_iszombie(thread) ||
        thread_isterminated(thread))
        return -EINVAL;

    if (thread_isstopped(thread) &&
        ((signo != SIGKILL) &&
         (signo != SIGCONT)))
        return -EINVAL;

    switch ((err = sigismember(&thread->t_sigmask, signo))) {
    case -EINVAL:
        break;
    case 0:
        thread->t_sigqueue[signo - 1]++;
        if (current != thread)
            err = thread_wake(thread);
        break;
    default:
        return 0;
    }

    return err;
}

int thread_sigdequeue(thread_t *thread) {
    int signo = 0;
    if (!thread)
        return -EINVAL;

    thread_assert_locked(thread);

    for ( ; signo < NSIG; ++signo) {
        if (thread->t_sigqueue[signo] >= 1) {
            if ((sigismember(&thread->t_sigmask, signo + 1)) == 1)
                return -EINVAL;
            thread->t_sigqueue[signo]--;
            signo += 1;
            return signo;
        }
    }

    return 0;
}

int thread_sigmask(thread_t *thread, int how, const sigset_t *restrict set, sigset_t *restrict oset) {
    int err = 0;
    thread_assert_locked(thread);
    
    if (oset)
        *oset = thread->t_sigmask;
    
    if (set == NULL)
        return 0;
    
    if (sigismember(set, SIGKILL) || sigismember(set, SIGSTOP))
        return -EINVAL;

    switch (how) {
    case SIG_BLOCK:
        thread->t_sigmask |= *set;
        break;
    case SIG_UNBLOCK:
        thread->t_sigmask &= ~*set;
        break;
    case SIG_SETMASK:
        thread->t_sigmask = *set;
        break;
    default:
        err = -EINVAL;
        break;
    }
    return err;
}

int thread_execve(proc_t *proc, thread_t *thread,
    thread_entry_t entry, const char *argp[], const char *envp[]) {
    int     err     = 0;
    int     argc    = 0;
    char    **arg   = NULL;
    char    **env   = NULL;
    vmr_t   *stack  = NULL;
    vmr_t   *tmp_stack = NULL;

    if (proc == NULL || thread == NULL || entry == NULL)
        return -EINVAL;

    // TODO: implement a function to reverse this.
    if ((err = mmap_argenvcpy(proc->mmap, (const char **)argp,
        (const char **)envp, &arg, &argc, &env)))
        return err;

    tmp_stack = thread->t_arch.t_ustack;

    if ((err = mmap_alloc_stack(proc->mmap, USTACKSZ, &stack)))
        goto error;

    thread->t_arch.t_ustack = stack;

    if ((err = arch_thread_execve(&thread->t_arch, entry, argc,
        (const char **)arg, (const char **)env)))
        goto error;

    return 0;
error:
    thread->t_arch.t_ustack = tmp_stack;

    //TODO: add here a call to reverse mmap_argenvcpy()

    mmap_remove(proc->mmap, stack);
    return err;
}

int builtin_threads_begin(size_t *nthreads) {
    int err = 0;
    builtin_thread_t *thrd = __builtin_thrds;
    size_t nr = __builtin_thrds_end - __builtin_thrds;

    for (size_t i = 0; i < nr; ++i, thrd++) {
        if ((err = kthread_create(NULL,
            (thread_entry_t)thrd->thread_entry,
            thrd->thread_arg, NULL)))
            return err;
    }

    if (nthreads)
        *nthreads = nr;
    return 0;
}

int thread_schedule(thread_t *thread) {
    thread_assert_locked(thread);
    return sched_park(thread);
}

int thread_fork(thread_t *dst, thread_t *src, mmap_t *mmap) {
    uintptr_t   sp      = 0;
    vmr_t       *stack  = NULL;

    if (dst == NULL || src == NULL)
        return -EINVAL;
    
    thread_assert_locked(dst);
    thread_assert_locked(src);

    sp = src->t_arch.t_tf->rsp;

    dst->t_sched = (thread_sched_t) {
        .ts_priority        = src->t_sched.ts_priority,
        .ts_processor       = src->t_sched.ts_processor,
        .ts_timeslice       = src->t_sched.ts_timeslice,
        .ts_affinity_type   = src->t_sched.ts_affinity_type,
    };

    if ((stack = mmap_find(mmap, sp)) == NULL) {
        return -EADDRNOTAVAIL;
    }

    dst->t_arch.t_ustack = stack;
    return arch_thread_fork(&dst->t_arch, &src->t_arch);
}