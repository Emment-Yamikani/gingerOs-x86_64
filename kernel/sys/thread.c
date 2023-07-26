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
static tgroup_t *kthreads = NULL;

extern char __builtin_thread_arg, __builtin_thread_arg_end;
extern char __builtin_thread_entry, __builtin_thread_entry_end;

const char *t_states[] = {
    [T_EMBRYO]      = "EMBRYO",
    [T_READY]       = "READY",
    [T_RUNNING]     = "RUNNING",
    [T_ISLEEP]      = "ISLEEP",
    [T_STOPPED]     = "STOPPED",
    [T_TERMINATED]  = "TERMINATED",
    [T_ZOMBIE]      = "ZOMBIE",
};

static tid_t tid_alloc(void) {
    return atomic_inc_fetch(&tids);
}

int thread_new(thread_t **ref) {
    int err = 0;
    cond_t *wait = NULL;
    uintptr_t kstack = 0;
    queue_t *queue = NULL;
    thread_t *thread = NULL;

    if (!(kstack = mapped_alloc(KSTACKSZ)))
        return -ENOMEM;
    
    thread = (thread_t *)ALIGN16((kstack + KSTACKSZ) - (sizeof *thread));

    if ((err = queue_new("thread", &queue)))
        goto error;

    if ((err = cond_new("thread", &wait)))
        goto error;

    memset(thread, 0, sizeof *thread);

    thread->t_arch = (x86_64_thread_t){
        .t_kstack = kstack,
    };

    thread->t_tid = tid_alloc();
    thread->t_lock = SPINLOCK_INIT();

    thread_lock(thread);

    thread->t_wait = wait;
    thread->t_queues = queue;

    thread->t_sched_attr.ctime = jiffies_get();
    thread->t_sched_attr.priority = SCHED_LOWEST_PRIORITY;

    thread_enter_state(thread, T_EMBRYO);

    *ref = thread;
    return 0;
error:
    if (wait)
        cond_free(wait);
    if (queue)
        queue_free(queue);
    mapped_free(kstack, KSTACKSZ);
    return err;
}

void thread_free(thread_t *thread) {
    queue_t *q = NULL;

    thread_assert(thread);
    assert(thread_embryo(thread) ||
                   thread_terminated(thread) ||
                   thread_zombie(thread),
               "freeing a non zombie thread");

    if (thread->t_queues) {
        queue_lock(thread->t_queues);
        while ((q = dequeue(thread->t_queues))) {
            queue_lock(q);
            queue_remove(q, thread);
            queue_unlock(q);
        }
        queue_unlock(thread->t_queues);
        queue_free(thread->t_queues);
    }

    assert(thread->t_arch.t_kstack, "No kernel stack ???");

    if (thread->t_wait)
        cond_free(thread->t_wait);
    if (thread_locked(thread))
        thread_unlock(thread);
    mapped_free(thread->t_arch.t_kstack, KSTACKSZ);
}

tid_t thread_self(void) {
    if (!current)
        return 0;
    return current->t_tid;
}

void thread_exit(uintptr_t exit_code) {
    current_assert();
    if (!tgroup_locked(current->t_group))
        tgroup_lock(current->t_group);
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
    thread_t *thread = NULL;
    assert(queue, "no queue");
    queue_assert_locked(queue);

    if (!(thread = dequeue(queue)))
        return NULL;
    thread_lock(thread);
    queue_lock(thread->t_queues);
    if (queue_remove(thread->t_queues, (void *)queue))
        panic("queue: \'%s\' not on \'%s\'\n", queue->name, thread->t_queues->name);
    queue_unlock(thread->t_queues);
    return thread;
}

int thread_remove_queue(thread_t *thread, queue_t *queue) {
    int err = 0;
    queue_assert_locked(queue);
    thread_assert_locked(thread);

    queue_lock(thread->t_queues);
    if ((err = queue_remove(thread->t_queues, (void *)queue)))
    {
        queue_unlock(thread->t_queues);
        return err;
    }
    queue_unlock(thread->t_queues);
    return queue_remove(queue, (void *)thread);
}

int thread_kill_n(thread_t *thread, int wait) {
    int err = 0;

    thread_assert_locked(thread);
    if (thread == current) {
        current_unlock();
        thread_exit(0);
    }

    if ((thread_terminated(thread)) ||
        (thread_zombie(thread)) ||
        thread_killed(thread))
        return 0;

    thread->t_killer = thread_self();
    thread_setflags(thread, THREAD_KILLED);

    if ((err = thread_wake(thread)))
        return err;

    if (wait)
        err = thread_wait(thread, 0, NULL);
    return err;
}

int thread_kill(tid_t tid, int wait) {
    int err = 0;
    tgroup_t *tgroup = NULL;

    if (tid < 0)
        return -EINVAL;

    current_lock();
    tgroup = current->t_group;
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

    if (thread_killed(current))
        return -EINTR;

    tgroup_lock(current->t_group);

    if ((err = tgroup_get_thread(current->t_group, tid, T_ZOMBIE, &thread))) {
        tgroup_unlock(current->t_group);
        return err;
    }

    tgroup_unlock(current->t_group);

    thread_assert_locked(thread);

    if ((err = thread_join_r(thread, info, retval))) {
        thread_unlock(thread);
        return err;
    }

    return 0;
}

int thread_join_r(thread_t *thread, thread_info_t *info, void **retval) {
    thread_assert_locked(thread);

    if (thread == NULL)
        return -EINVAL;

    if (info) {
        info->ti_errno = thread->t_errno;
        info->ti_exit = thread->t_exit;
        info->ti_flags = thread->t_flags;
        info->ti_killer = thread->t_killer;
        info->ti_sched = thread->t_sched_attr;
        info->ti_state = thread->t_state;
        info->ti_tid = thread->t_tid;
    }

    if (thread == current)
        return -EDEADLOCK;

        

    loop() {
        if (thread_killed(current))
            return -EINTR;

        if (thread->t_state == T_ZOMBIE)
        {
            if (retval) *retval = (void *)thread->t_exit;
            thread_free(thread);
            break;
        }

        thread_unlock(thread);
        cond_wait(thread->t_wait);
        thread_lock(thread);
    }

    return 0;
}

int thread_wait(thread_t *thread, int reap, void **retval) {
    int err = 0;
    (void)err;
    thread_assert_locked(thread);

    loop()
    {
        if (thread_killed(current))
            return -EINTR;

        if ((thread->t_state == T_ZOMBIE) || (thread->t_state == T_TERMINATED))
        {
            if (retval)
                *retval = (void *)thread->t_exit;
            if (reap)
                thread_free(thread);
            break;
        }

        thread_unlock(thread);
        if ((err = cond_wait(thread->t_wait)))
        {
            thread_lock(thread);
            return err;
        }
        thread_lock(thread);
    }

    return 0;
}

int thread_kill_all(void) {
    int err = 0;
    current_assert();
    tgroup_lock(current->t_group);
    err = tgroup_kill_thread(current->t_group, 0, 1);
    tgroup_unlock(current->t_group);
    return err;
}

int thread_wake(thread_t *thread) {
    int err = 0, guard_locked = 0, q_locked = 0;
    thread_assert_locked(thread);

    if (thread_testflags(thread, THREAD_SETPARK))
        thread_setflags(thread, THREAD_SETWAKE);

    if (thread_zombie(thread) ||
        !thread_isleep(thread) ||
        thread_terminated(thread))
        return 0;
    
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

    if (thread_zombie(thread) || thread_terminated(thread))
        return 0;

    if ((err = thread_enter_state(thread, T_READY)))
        return err;

    return sched_park(thread);
}

int thread_queue_get(queue_t *queue, tid_t tid, thread_t **tref) {
    thread_t *thread = NULL;
    queue_node_t *next = NULL;

    if (!queue || !tref)
        return -EINVAL;

    if (tid < 0)
        return -EINVAL;

    if (tid == thread_self())
    {
        *tref = current;
        return 0;
    }

    queue_assert_locked(queue);
    forlinked(node, queue->head, next)
    {
        next = node->next;
        thread = node->data;

        thread_lock(thread);
        if (thread->t_tid == tid)
        {
            *tref = thread;
            return 0;
        }
        thread_unlock(thread);
    }

    return -ESRCH;
}

int kthread_create(void *(*entry)(void *), void *arg, tid_t *__tid, thread_t **ref) {
    int err = 0;
    thread_t *thread = NULL;

    if ((err = thread_new(&thread)))
        return err;

    if ((err = arch_kthread_init(&thread->t_arch, entry, arg)))
        goto error;

    if (kthreads == NULL)
    {
        // assert(!(err = f_alloc_table(&file_table)), "couldn't allocate file table");
        assert(!(err = tgroup_create(thread, &kthreads)), "failed to create tgroup");
        tgroup_unlock(kthreads);
    } else {
        tgroup_lock(kthreads);
        if ((err = tgroup_add_thread(kthreads, thread))) {
            tgroup_unlock(kthreads);
            goto error;
        }
        tgroup_unlock(kthreads);
    }

    if (__tid)
        *__tid = thread->t_tid;
    if (ref)
        *ref = thread;

    err = sched_park(thread);
    thread_unlock(thread);
    return err;
error:
    if (thread)
        thread_free(thread);
    return err;
}

int kthread_create_join(void *(*entry)(void *), void *arg, void **ret) {
    int err = 0;
    assert(entry, "no entry point");
    tid_t tid = 0;
    if ((err = kthread_create(entry, arg, &tid, NULL)))
        return err;
    return thread_join(tid, NULL, ret);
}

int start_builtin_threads(int *nthreads, thread_t ***threads) {
    int nt = 0;
    int err = 0;
    size_t nr = 0;
    thread_t *thread = NULL;
    thread_t **builtin_threads = NULL;

    void **argv = (void **)&__builtin_thread_arg;
    void **entryv = (void **)&__builtin_thread_entry;
    void **end = (void **)&__builtin_thread_entry_end;

    nr = (end - entryv);

    for (size_t i = 0; i < nr; ++i)
    {
        void *arg = argv[i];
        void *(*entry)(void *) = entryv[i];

        if (entry)
        {
            if (builtin_threads == NULL)
            {
                if ((builtin_threads = kmalloc(sizeof(thread_t *))) == NULL)
                {
                    err = -ENOMEM;
                    break;
                }
            }
            else if ((builtin_threads = krealloc(builtin_threads, (sizeof(thread_t *) * (nt + 1)))) == NULL)
            {
                err = -ENOMEM;
                break;
            }

            if ((err = kthread_create(entry, arg, NULL, &thread)))
                break;

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