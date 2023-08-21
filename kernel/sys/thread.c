#include <lib/stdint.h>
#include <sys/thread.h>
#include <sys/sched.h>
#include <bits/errno.h>
#include <mm/kalloc.h>
#include <mm/vmm.h>
#include <sys/system.h>
#include <lib/string.h>
#include <ginger/jiffies.h>
#include <arch/x86_64/thread.h>

static atomic_t tids = {0};
static atomic_t pids = {0};

extern char __builtin_thread_arg, __builtin_thread_arg_end;
extern char __builtin_thread_entry, __builtin_thread_entry_end;

const char *t_states[] = {
    [T_EMBRYO]      = "EMBRYO",
    [T_READY]       = "READY",
    [T_RUNNING]     = "RUNNING",
    [T_ISLEEP]      = "ISLEEP",
    [T_STOPPED]     = "STOPPED",
    [T_USLEEP]      = "USLEEP",
    [T_TERMINATED]  = "TERMINATED",
    [T_ZOMBIE]      = "ZOMBIE",
};

static tid_t tid_alloc(void) {
    return atomic_inc_fetch(&tids);
}

uintptr_t thread_alloc_kstack(size_t size) {
    return (uintptr_t)kmalloc(size);
}

void thread_free_kstack(uintptr_t addr, size_t size __unused) {
    kfree((void *)addr);
}

int thread_new(thread_attr_t *attr, thread_entry_t entry, void *arg, int flags, thread_t **ref) {
    int         err         = 0;
    uintptr_t   kstack      = 0;
    thread_attr_t t_attr    = {0};
    cond_t      *wait       = NULL;
    queue_t     *queue      = NULL;
    thread_t    *thread     = NULL;
    size_t      kstacksz    = KSTACKSZ;

    if (attr == NULL) {
        if (!(kstack = thread_alloc_kstack(kstacksz)))
            return -ENOMEM;
        
        t_attr = (thread_attr_t) {
            .detachstate   = 0,
            .guardsz        = 0,
            .stackaddr      = kstack,
            .stacksz        = kstacksz,
        };
        attr = &t_attr;
    } else {
        if (BADSTACKSZ(attr->stacksz))
            return -EINVAL;

        if (!BTEST(flags, 0)) {
            if (attr->stackaddr == 0)
                if (!(attr->stackaddr = thread_alloc_kstack(attr->stacksz)))
                    return -ENOMEM;
            kstack = attr->stackaddr;
            kstacksz = attr->stacksz;
        } else {
            if (attr->stackaddr == 0)
                return -EINVAL;
            if (!(kstack = thread_alloc_kstack(kstacksz)))
                return -ENOMEM;
        }

        t_attr = *attr;
    }

    thread = (thread_t *)ALIGN16((kstack + kstacksz) - (sizeof *thread));

    if ((err = queue_new("thread", &queue)))
        goto error;

    if ((err = cond_new("thread", &wait)))
        goto error;

    memset(thread, 0, sizeof *thread);

    thread->t_arch = (x86_64_thread_t) {
        .t_kstack = kstack,
        .t_kstacksz = kstacksz,
    };

    thread->t_attr = t_attr;
    thread->t_tid = tid_alloc();
    thread->t_lock = SPINLOCK_INIT();

    thread_lock(thread);

    if (attr->detachstate)
        thread_setdetached(thread);

    thread->t_wait = wait;
    thread->t_queues = queue;

    thread->t_sched_attr.ctime = jiffies_get();
    thread->t_sched_attr.priority = SCHED_LOWEST_PRIORITY;

    thread_enter_state(thread, T_EMBRYO);

    if ((err = arch_thread_init(&thread->t_arch, entry, arg)))
        goto error;

    *ref = thread;
    return 0;
error:
    if (wait)
        cond_free(wait);
    if (queue)
        queue_free(queue);
    thread_free_kstack(kstack, kstacksz);
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

    if (thread->t_wait)
        cond_free(thread->t_wait);
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

    if (!(node = enqueue(queue, (void *)thread)))
    {
        err = -ENOMEM;
        queue_unlock(thread->t_queues);
        queue_unlock(queue);
        goto error;
    }

    if (!enqueue(thread->t_queues, (void *)queue))
    {
        err = -ENOMEM;
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

    if (!(thread = dequeue(queue)))
        return NULL;
    thread_lock(thread);
    queue_lock(thread->t_queues);
    if ((err = queue_remove(thread->t_queues, (void *)queue)))
        panic("queue: \'%s\' not on \'%s\', error: %d\n", queue->name, thread->t_queues->name, err);
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
        if (current_killed())
            return -EINTR;
        thread_unlock(thread);
        err = cond_wait(thread->t_wait);
        thread_lock(thread);
        if (err) return err;
    }

    if (info) {
        info->ti_tid    = thread->t_tid;
        info->ti_exit   = thread->t_exit;
        info->ti_flags  = thread->t_flags;
        info->ti_errno  = thread->t_errno;
        info->ti_state  = thread->t_state;
        info->ti_killer = thread->t_killer;
        info->ti_sched  = thread->t_sched_attr;
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

    if (current_killed())
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

    if (thread_iszombie(thread) ||
        thread_isterminated(thread))
        return 0;

    if (thread_testflags(thread, THREAD_SETPARK))
        thread_setflags(thread, THREAD_SETWAKE);

    if (thread->sleep_attr.queue == NULL)
        return 0;

    if (thread->sleep_attr.guard && (guard_locked = !spin_locked(thread->sleep_attr.guard)))
        spin_lock(thread->sleep_attr.guard);

    if ((q_locked = !queue_locked(thread->sleep_attr.queue)))
        queue_lock(thread->sleep_attr.queue);

    err = thread_remove_queue(thread, thread->sleep_attr.queue);

    if (q_locked)
        queue_unlock(thread->sleep_attr.queue);

    if (thread->sleep_attr.guard && guard_locked)
        spin_unlock(thread->sleep_attr.guard);

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

    if (!thread)
        return -EINVAL;

    thread_assert_locked(thread);
    switch ((err = sigismember(&thread->t_sigmask, signo)))
    {
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

int builtin_threads_begin(int *nthreads, thread_t ***threads) {
    int nt = 0;
    int err = 0;
    size_t nr = 0;
    thread_t *thread = NULL;
    thread_t **builtin_threads = NULL;

    void **argv = (void **)&__builtin_thread_arg;
    void **entryv = (void **)&__builtin_thread_entry;
    void **end = (void **)&__builtin_thread_entry_end;

    nr = (end - entryv);

    for (size_t i = 0; i < nr; ++i) {
        void *arg = argv[i];
        void *(*entry)(void *) = entryv[i];

        thread_attr_t t_attr = {
            .detachstate = 0,
            .guardsz = 0,
            .stackaddr = 0,
            .stacksz = STACKSZMIN,
        };

        if (entry) {
            if (builtin_threads == NULL) {
                if ((builtin_threads = kmalloc(sizeof(thread_t *))) == NULL) {
                    err = -ENOMEM;
                    break;
                }
            }
            else if ((builtin_threads = krealloc(builtin_threads, (sizeof(thread_t *) * (nt + 1)))) == NULL) {
                err = -ENOMEM;
                break;
            }

            if ((err = thread_create(&thread, &t_attr, entry, arg)))
                break;

            thread_unlock(thread);
            builtin_threads[nt++] = thread;
        }
    }

    *nthreads = nt;
    if (threads)
        *threads = builtin_threads;
    else
        kfree(builtin_threads);
    return err;
}

int thread_create(thread_t **pthread, thread_attr_t *attr, thread_entry_t entry, void *arg) {
    int err = 0;
    int user = 0;
    int newgroup = 0;
    thread_t *thread = NULL;
    tgroup_t *tgroup = current ? current_tgroup() : NULL;

    newgroup = !tgroup;

    if (!entry)
        return -EINVAL;

    if (current) {
        current_lock();
        user = current_isuser();
        current_unlock();
    }

    if (tgroup)
        tgroup_lock(tgroup);
    else if ((err = tgroup_create(&tgroup)))
        goto error;

    if ((err = thread_new(attr, entry, arg, user, &thread))) {
        tgroup_unlock(tgroup);
        goto error;
    }

    if ((err = tgroup_add_thread(tgroup, thread))) {
        tgroup_unlock(tgroup);
        goto error;
    }

    if ((err = thread_schedule(thread))) {
        tgroup_unlock(tgroup);
        goto error;
    }
    
    if (pthread)
        *pthread = thread;
    else
        thread_unlock(thread);

    tgroup_unlock(tgroup);

    return 0;
error:
    if (newgroup && tgroup) {
        tgroup_lock(tgroup);
        tgroup_destroy(tgroup);
    }
    
    if (thread)
        thread_free(thread);
    
    return err;
}

int thread_schedule(thread_t *thread) {
    thread_assert_locked(thread);
    return sched_park(thread);
}