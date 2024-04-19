#include <arch/thread.h>
#include <bits/errno.h>
#include <ginger/jiffies.h>
#include <lib/stdint.h>
#include <lib/string.h>
#include <mm/kalloc.h>
#include <mm/vmm.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/system.h>
#include <sys/thread.h>

const char *t_states[] = {
    [T_EMBRYO]      = "EMBRYO",
    [T_READY]       = "READY",
    [T_RUNNING]     = "RUNNING",
    [T_ISLEEP]      = "ISLEEP",
    [T_USLEEP]      = "USLEEP",
    [T_STOPPED]     = "STOPPED",
    [T_ZOMBIE]      = "ZOMBIE",
    [T_TERMINATED]  = "TERMINATED",
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
     * in which tid was has terminated completely.
     */
    static atomic_t tids = {0};
    return atomic_inc_fetch(&tids);
}

int thread_kstack_alloc(size_t size, uintptr_t *ret) {
    uintptr_t addr = 0;

    if (ret == NULL)
        return -EINVAL;
    
    if (BADSTACKSZ(size))
        return -ERANGE;
    
    if (0 == (addr = (uintptr_t)kmalloc(size)))
        return -ENOMEM;
    
    *ret = addr;
    return 0;
}

void thread_kstack_free(uintptr_t addr) {
    if (addr == 0)
        return;
    kfree((void *)addr);
}

/**
 * @brief allocate a new thread structure.
 *
 * @param kstacksz size of the new thread's kernel stack.
 * @param flags flags used in thread creation, e.g THREAD_USER(if a user thread is intended).
 * @param ref the new structure is passed by reference through a pointer to a thread pointer.
 * @return int 0 on success otherwise an error code is passed.
 */
int thread_alloc(size_t ksz /*kstacksz*/, int __flags, thread_t **ret) {
    int             err     = 0;
    int             flags   = 0;
    uintptr_t       kstack  = 0;
    arch_thread_t   *arch   = NULL;
    thread_t        *thread = NULL;
    thread_sched_t  *sched  = NULL;

    if (ret == NULL)
        return -EINVAL;

    if ((err = thread_kstack_alloc(ksz, &kstack)))
        return err;

    thread = (thread_t *)ALIGN16((kstack + ksz) - sizeof *thread);
    memset(thread, 0, sizeof *thread);
    thread->t_lock  = SPINLOCK_INIT();
    thread_lock(thread);

    sched                   = &thread->t_sched;
    arch                    = &thread->t_arch;

    arch->t_kstack.ss_size  = ksz;
    arch->t_thread          = thread;
    arch->t_kstack.ss_sp    = (void *)kstack;
    arch->t_sstack.ss_sp    = (void *)thread;
    /**
     * 1Kib should be large enough
     * to act as a scratch space for
     * executing thread for the first time.
    */
    arch->t_sstack.ss_size  = (usize)KiB(1);
    arch->t_rsvd            = (void *)thread - KiB(1);

    thread->t_refcnt        = 2;
    thread->t_tid           = tid_alloc();
    thread->t_queues        = QUEUE_INIT();
    
    sched->ts_affinity.cpu_set = -1;
    sched->ts_ctime            = jiffies_get();
    sched->ts_affinity.type    = SOFT_AFFINITY;
    sched->ts_priority         = SCHED_LOWEST_PRIORITY;

    // every thread begins as an embryo.
    thread_enter_state(thread, T_EMBRYO);

    // is thread user thread?
    flags |= __flags & THREAD_CREATE_USER ? THREAD_USER : 0;

    // is thread self detaching?
    flags |= __flags & THREAD_CREATE_DETACHED ? THREAD_DETACHED : 0;

    thread_setflags(thread, flags); // set the flags.
    *ret = thread;
    return 0;
}

int kthread_create(thread_attr_t *__attr, thread_entry_t entry, void *arg, int flags, thread_t **ret) {
    int             err         = 0;
    thread_attr_t   attr        = {0};
    thread_t        *thread     = NULL;
    
    attr = __attr ? *__attr : (thread_attr_t) {
        .detachstate = 0,
        .stackaddr   = 0,
        .guardsz     = PAGESZ,
        .stacksz     = KSTACKSZ
    };

    // this is only used for kernel thread creation.
    if (flags & THREAD_CREATE_USER)
        return -EINVAL;

    flags |= current == NULL  ? THREAD_CREATE_GROUP    : 0;

    // create a self detatching thread.
    flags |= attr.detachstate ? THREAD_CREATE_DETACHED : 0;

    // allocate the thread struct and kernel struct.
    if ((err = thread_alloc(attr.stacksz, flags, &thread)))
        return err;

    // set the thread's entry point.
    thread->t_entry  = entry;

    // Initialize the kernel execution context.
    if ((err = arch_kthread_init(&thread->t_arch, entry, arg)))
        goto error;

    // Do we want to create a new thread group?
    if (flags & THREAD_CREATE_GROUP) {
        // If so create a thread group and make thread the main thread.
        if ((err = thread_create_group(thread)))
            goto error;
    } else {
        if ((err = thread_join_group(thread)))
            goto error;
    }

    // schedule the newly created thread?
    if (flags & THREAD_CREATE_SCHED) {
        if ((err = thread_schedule(thread)))
            goto error;
    }

    if (ret)
        *ret = thread;
    else {
        thread->t_refcnt--;
        thread_unlock(thread);
    }

    return 0;
error:
    if (thread)
        thread_free(thread);
    return err;
}

int thread_create(thread_attr_t *attr, thread_entry_t entry, void *arg, thread_t **pthread) {
    int             err         = 0;
    thread_attr_t   t_attr      = {0};
    uc_stack_t      uc_stack    = {0};
    vmr_t           *ustack     = NULL;
    thread_t        *thread     = NULL;

    t_attr = attr ? *attr : (thread_attr_t){
        .detachstate    = 0,
        .stackaddr      = 0,
        .guardsz        = PGSZ,
        .stacksz        = USTACKSZ,
    };

    if (curproc == NULL)
        return -EINVAL;

    if ((err = thread_alloc(KSTACKSZ, THREAD_CREATE_USER, &thread)))
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

    uc_stack.ss_size    = __vmr_size(ustack);
    uc_stack.ss_flags   = __vmr_vflags(ustack);
    uc_stack.ss_sp      = (void *)__vmr_start(ustack);

    thread->t_mmap = proc_mmap(curproc);
    proc_mmap_unlock(curproc);
    proc_unlock(curproc);

    /// TODO: Optmize size of ustack according to attr;
    /// TODO: maybe perform a split? But then this will mean
    /// free() and unmap() calls to reverse malloc() and mmap() respectively
    /// for this region may fail. ???
    thread->t_arch.t_ustack = uc_stack;

    thread->t_owner = curproc;

    if ((err = arch_uthread_init(&thread->t_arch, entry, arg)))
        goto error;

    if ((err = thread_join_group(thread))) {
        goto error;
    }

    if (pthread)
        *pthread = thread;
    else
        thread_unlock(thread);
    return 0;
error:
    if (thread) thread_free(thread);
    return err;
}

void thread_free(thread_t *thread) {
    queue_t *queue = NULL;
    queue_node_t *next = NULL;

    assert (current != thread, "current called freeing it's own kstack???");

    thread_assert_locked(thread);
    assert(thread_iszombie(thread), "freeing a non zombie thread");

    /**
     * Get rid of all the queues with which this thread is associated.
    */
    queue_lock(&thread->t_queues);
    forlinked(node, thread->t_queues.head, next) {
        next = node->next;
        queue = (queue_t *)node->data;
        // queue_unlock(&thread->t_queues);
        queue_lock(queue);
        // queue_lock(&thread->t_queues);
        if (queue_remove(queue, thread))
            panic("failed to remove thread from queue\n");

        if (queue_remove(&thread->t_queues, queue))
            panic("failed to remove queue from thread-queue\n");

        queue_unlock(queue);
    }
    queue_unlock(&thread->t_queues);

    assert(thread->t_arch.t_kstack.ss_sp, "??? No kernel stack ???");

    thread_unlock(thread);
    thread_kstack_free((uintptr_t)thread->t_arch.t_kstack.ss_sp);
}

int thread_detach(thread_t *thread) {
    return thread_leave_group(thread);
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
    queue_lock(&thread->t_queues);

    if ((err = enqueue(queue, (void *)thread, 1, NULL))) {
        queue_unlock(&thread->t_queues);
        queue_unlock(queue);
        goto error;
    }

    if ((err = enqueue(&thread->t_queues, (void *)queue, 1, NULL))) {
        node = NULL;
        queue_remove(queue, (void *)thread);
        queue_unlock(&thread->t_queues);
        queue_unlock(queue);
        goto error;
    }

    queue_unlock(&thread->t_queues);
    queue_unlock(queue);

    if (rnode)
        *rnode = node;

    return 0;
error:
    printk("fialed to enqueue thread, error: %d\n", err);
    return err;
}

thread_t *thread_dequeue(queue_t *queue) {
    int     err = 0;
    thread_t *thread = NULL;
    assert(queue, "no queue");
    queue_assert_locked(queue);

    if ((err = dequeue(queue, (void **)&thread)))
        return NULL;
    thread_lock(thread);
    queue_lock(&thread->t_queues);
    if ((err = queue_remove(&thread->t_queues, (void *)queue)))
        panic("queue is not on thread[%d]'s threads-queue, error: %d\n", thread_gettid(thread), err);
    queue_unlock(&thread->t_queues);
    return thread;
}

int thread_remove_queue(thread_t *thread, queue_t *queue) {
    int err = 0;
    queue_assert_locked(queue);
    thread_assert_locked(thread);

    queue_lock(&thread->t_queues);
    if ((err = queue_remove(&thread->t_queues, (void *)queue))) {
        queue_unlock(&thread->t_queues);
        return err;
    }
    queue_unlock(&thread->t_queues);
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
    int     err     = 0;
    queue_t *tgroup = NULL;

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

    if (thread->t_sleep.queue == NULL)
        return 0;

    if (thread->t_sleep.guard && (guard_locked = !spin_islocked(thread->t_sleep.guard)))
        spin_lock(thread->t_sleep.guard);

    if ((q_locked = !queue_islocked(thread->t_sleep.queue)))
        queue_lock(thread->t_sleep.queue);

    err = thread_remove_queue(thread, thread->t_sleep.queue);

    if (q_locked)
        queue_unlock(thread->t_sleep.queue);

    if (thread->t_sleep.guard && guard_locked)
        spin_unlock(thread->t_sleep.guard);

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

int thread_sigqueue(thread_t *thread, siginfo_t *info) {
    int     err = 0;

    if (!thread || SIGBAD(info->si_signo))
        return -EINVAL;

    thread_assert_locked(thread);

    if (thread_iszombie(thread) ||
        thread_isterminated(thread))
        return -EINVAL;

    if (thread_isstopped(thread) && (
        (info->si_signo != SIGKILL) &&
        (info->si_signo != SIGCONT)))
        return -EINVAL;

    switch ((err = sigismember(&thread->t_sigmask, info->si_signo))) {
    case -EINVAL:
        break;
    case 0:
        queue_lock(&thread->t_sigqueue[info->si_signo - 1]);
        enqueue(&thread->t_sigqueue[info->si_signo - 1], info, 1, NULL);
        queue_unlock(&thread->t_sigqueue[info->si_signo - 1]);
    
        if (err != 0)
            return err;

        if (current != thread)
            err = thread_wake(thread);
        break;
    default:
        return 0;
    }

    return err;
}

int thread_sigdequeue(thread_t *thread, siginfo_t **ret) {
    int         signo = 0;
    siginfo_t   *info = NULL;

    if (!thread || !ret)
        return -EINVAL;

    thread_assert_locked(thread);

    for ( ; signo < NSIG; ++signo) {
        queue_lock(&thread->t_sigqueue[signo]);
        if (queue_count(&thread->t_sigqueue[signo])) {
            if ((sigismember(&thread->t_sigmask, signo + 1)) == 1) {
                queue_unlock(&thread->t_sigqueue[signo]);
                return -EINVAL;
            }
            
            sigdequeue_pending(&thread->t_sigqueue[signo], &info);
            queue_unlock(&thread->t_sigqueue[signo]);
            // sigaddset(&thread->t_sigmask, signo + 1);
            *ret = info;
            return 0;
        }

        queue_unlock(&thread->t_sigqueue[signo]);
    }

    return -ENOENT;
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

int thread_execve(proc_t *proc, thread_t *thread, thread_entry_t entry, const char *argp[], const char *envp[] ) {
    int     err     = 0;
    int     argc    = 0;
    char    **arg   = NULL;
    char    **env   = NULL;
    vmr_t   *ustack  = NULL;
    uc_stack_t tmp_stack = {0};
    uc_stack_t uc_stack = {0};

    if (proc == NULL || thread == NULL || entry == NULL)
        return -EINVAL;

    // TODO: implement a function to reverse this.
    if ((err = mmap_argenvcpy(proc->mmap, (const char **)argp,
        (const char **)envp, &arg, &argc, &env)))
        return err;

    tmp_stack = thread->t_arch.t_ustack;

    if ((err = mmap_alloc_stack(proc->mmap, USTACKSZ, &ustack)))
        goto error;

    uc_stack.ss_size    = __vmr_size(ustack);
    uc_stack.ss_flags   = __vmr_vflags(ustack);
    uc_stack.ss_sp      = (void *)__vmr_start(ustack);

    thread->t_arch.t_ustack = uc_stack;

    if ((err = arch_thread_execve(&thread->t_arch, entry, argc,
        (const char **)arg, (const char **)env)))
        goto error;

    return 0;
error:
    thread->t_arch.t_ustack = tmp_stack;

    //TODO: add here a call to reverse mmap_argenvcpy()

    mmap_remove(proc->mmap, ustack);
    return err;
}

int builtin_threads_begin(size_t *nthreads) {
    int                 err     = 0;
    builtin_thread_t    *thrd   = __builtin_thrds;
    size_t              nr      = __builtin_thrds_end - __builtin_thrds;

    for (size_t i = 0; i < nr; ++i, thrd++) {
        if ((err = kthread_create(NULL,
            (thread_entry_t)thrd->thread_entry,
            thrd->thread_arg, THREAD_CREATE_SCHED, NULL)))
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
    uintptr_t   sp          = 0;
    uc_stack_t  uc_stack    = {0};
    vmr_t       *ustack     = NULL;
    mcontext_t  *mctx       = NULL;

    if (dst == NULL || src == NULL)
        return -EINVAL;
    
    thread_assert_locked(dst);
    thread_assert_locked(src);

    mctx = &src->t_arch.t_uctx->uc_mcontext;
    sp = mctx->rsp;

    dst->t_sched = (thread_sched_t) {
        .ts_priority        = src->t_sched.ts_priority,
        .ts_processor       = src->t_sched.ts_processor,
        .ts_timeslice       = src->t_sched.ts_timeslice,
        .ts_affinity.type   = src->t_sched.ts_affinity.type,
    };

    if ((ustack = mmap_find(mmap, sp)) == NULL) {
        return -EADDRNOTAVAIL;
    }

    uc_stack.ss_size    = __vmr_size(ustack);
    uc_stack.ss_flags   = __vmr_vflags(ustack);
    uc_stack.ss_sp      = (void *)__vmr_start(ustack);
    dst->t_arch.t_ustack = uc_stack;
    return arch_thread_fork(&dst->t_arch, &src->t_arch);
}