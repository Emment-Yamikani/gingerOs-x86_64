#include <lib/stdint.h>
#include <sys/thread.h>
#include <sys/sched.h>
#include <bits/errno.h>
#include <mm/kalloc.h>
#include <mm/vmm.h>
#include <sys/system.h>
#include <lib/string.h>
#include <arch/x86_64/thread.h>

static atomic_t tids = {0};
static atomic_t pids = {0};
static tgroup_t *kthreads = NULL;

static tid_t tid_alloc(void) { return atomic_inc_fetch(&tids); }

int thread_new(thread_t **ref) {
    int err = 0;
    cond_t *wait = NULL;
    uintptr_t kstack = 0;
    queue_t *queue = NULL;
    thread_t *thread = NULL;

    if (!(kstack = mapped_alloc(KSTACKSZ)))
        return -ENOMEM;
    
    thread = (thread_t *)ALIGN16MB((kstack + KSTACKSZ) - (sizeof *thread));

    if ((err = queue_new("thread", &queue)))
        goto error;

    if ((err = cond_new("thread", &wait)))
        goto error;

    memset(thread, 0, sizeof *thread);

    thread->t_arch = (x86_64_thread_t){
        .t_kstack = kstack,
    };

    thread->t_wait = wait;
    thread->t_queues = queue;
    thread->t_tid = tid_alloc();
    thread->t_lock = SPINLOCK_INIT();
    thread->t_sched_attr.priority = SCHED_LOWEST_PRIORITY;

    thread_lock(thread);
    __thread_enter_state(thread, T_EMBRYO);

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
    assert(__thread_embryo(thread) ||
                   __thread_terminated(thread) ||
                   __thread_zombie(thread),
               "freeing a non zombie thread");

    if (thread->t_queues)
    {
        queue_lock(thread->t_queues);
        while ((q = dequeue(thread->t_queues)))
        {
            queue_lock(q);
            queue_remove(q, thread);
            queue_unlock(q);
        }
        queue_free(thread->t_queues);
    }

    assert(thread->t_arch.t_kstack, "No kernel stack ???");

    if (thread->t_wait)
        cond_free(thread->t_wait);
    mapped_free(thread->t_arch.t_kstack, KSTACKSZ);
}

void tgroup_free(tgroup_t *tgroup)
{
    thread_t *thread = NULL;
    assert(tgroup, "no tgroup pointer");
    if (tgroup->queue)
    {
        queue_lock(tgroup->queue);
        while ((thread = thread_dequeue(tgroup->queue)))
        {
            printk("\e[07;04mtrashing thread(%d)\e[0m\n", thread->t_tid);
            thread_free(thread);
        }
        queue_free(tgroup->queue);
    }
    kfree(tgroup);
}

int tgroup_new(tid_t tgid, tgroup_t **ref)
{
    int err =0;
    queue_t *queue = NULL;
    tgroup_t *tgroup = NULL;

    assert(ref, "no tgroup poiter reference");
    if (!(tgroup = kmalloc(sizeof *tgroup)))
        return -ENOMEM;

    if ((err = queue_new("tgroup", &queue)))
        goto error;

    memset(tgroup, 0, sizeof *tgroup);

    tgroup->gid = tgid;
    tgroup->queue = queue;

    *ref = tgroup;
    return 0;

error:
    if (tgroup)
        kfree(tgroup);
    if (queue)
        queue_free(queue);
    printk("failed to create task group, error: %d\n", err);
    return err;
}

void tgroup_wait_all(tgroup_t *tgroup)
{
    thread_t *thread = NULL;
    queue_node_t *next = NULL;

    if (tgroup == NULL)
        return;
    
    queue_lock(tgroup->queue);

    forlinked(node, tgroup->queue->head, next)
    {
        next = node->next;
        thread = node->data;

        if (thread == current)
            continue;

        thread_lock(thread);
        thread_kill_n(thread);
        
        if (thread->sleep_attr.queue) {
            queue_lock(thread->sleep_attr.queue);
            thread_wake_n(thread);
            queue_unlock(thread->sleep_attr.queue);
        }     
       
        queue_unlock(tgroup->queue);
        thread_wait(thread, 0, NULL);
        queue_lock(tgroup->queue);

        thread_unlock(thread);
    }

    queue_unlock(tgroup->queue);

    return;
}

int tgroup_kill_thread(tgroup_t *tgroup, tid_t tid)
{
    int err = 0;
    thread_t *thread = NULL;
    queue_node_t *next = NULL;

    if (tgroup == NULL)
        return -EINVAL;
    
    if (current && __thread_killed(current))
        return -EALREADY;

    if (tid == -1)
        goto all;
    else if (tid == 0)
        thread_exit(0);

    if ((err = thread_get(tgroup, tid, &thread)))
        return err;

    thread_assert_locked(thread);
    if (thread == current)
    {
        current_unlock();
        thread_exit(0);
    }

    if ((err = thread_kill_n(thread)))
    {
        thread_unlock(thread);
        return err;
    }
    thread_unlock(thread);

    return 0;
all:
    queue_lock(tgroup->queue);

    forlinked(node, tgroup->queue->head, next)
    {
        err = 0;
        next = node->next;
        thread = node->data;

        if (thread == current)
            continue;

        thread_lock(thread);
        thread_kill_n(thread);

        if (thread->sleep_attr.queue) {
            queue_lock(thread->sleep_attr.queue);
            thread_wake_n(thread);
            queue_unlock(thread->sleep_attr.queue);
        }

        queue_unlock(tgroup->queue);
        err = thread_wait(thread, 1, NULL);
        queue_lock(tgroup->queue);
        if (err) {
            thread_unlock(thread);
            break;
        }
    }

    queue_unlock(tgroup->queue);
    return err;
}

tid_t thread_self(void)
{
    current_assert();
    return current->t_tid;
}

void thread_exit(uintptr_t exit_code)
{
    current_assert();
    arch_thread_exit(exit_code);
}

int thread_enqueue(queue_t *queue, thread_t *thread, queue_node_t **rnode)
{
    int err = 0;
    queue_node_t *node = NULL;

    assert(queue, "no queue");
    assert(thread, "no thread");

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

thread_t *thread_dequeue(queue_t *queue)
{
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

int thread_remove_queue(thread_t *thread, queue_t *queue)
{
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

int thread_wake(thread_t *thread)
{
    return thread_wake_n(thread);
}

int thread_get(tgroup_t *tgrp, tid_t tid, thread_t **tref)
{
    thread_t *thread = NULL;
    assert(tgrp, "no tgrp");
    assert(tref, "no thread reference");
    assert(tgrp->queue, "no tgrp->queue");

    queue_lock(tgrp->queue);
    forlinked(node, tgrp->queue->head, node->next)
    {
        thread = node->data;
        if (thread->t_tid == tid)
        {
            *tref = thread;
            thread_lock(thread);
            queue_unlock(tgrp->queue);
            return 0;
        }
    }
    queue_unlock(tgrp->queue);

    return -ENOENT;
}

int thread_kill_n(thread_t *thread)
{
    thread_assert(thread);
    if (thread == current)
        return 0;
    thread_assert_locked(thread);
    if ((__thread_zombie(thread)) ||
        (__thread_terminated(thread)) ||
        __thread_killed(thread))
        return 0;
    __thread_setflags(thread, THREAD_KILLED);
    atomic_write(&thread->t_killer, thread_self());
    return 0;
}

int thread_kill(tid_t tid)
{
    tgroup_t *tgroup = NULL;

    if (tid < 0)
        return -EINVAL;

    current_lock();
    tgroup = current->t_group;
    current_unlock();

    return tgroup_kill_thread(tgroup, tid);
}

int thread_join(tid_t tid, void **retval)
{
    int err = 0;
    thread_t *thread = NULL;
    if (tid < 0)
        return -EINVAL;

    printk("%s(%d, %p)\n", __func__, thread_self(), tid, retval);

    current_assert();

    if (__thread_killed(current))
        return -EINTR;

    assert(current->t_group, "no t_group");
    if ((err = thread_get(current->t_group, tid, &thread)))
        return err;

    thread_assert_locked(thread);
    if (thread == current)
    {
        thread_unlock(thread);
        return -EDEADLOCK;
    }

    for (;;)
    {
        if (__thread_killed(current))
            return -EINTR;

        if (thread->t_state == T_ZOMBIE)
        {
            if (retval)
                *retval = (void *)thread->t_exit;
            thread_free(thread);
            break;
        }

        thread_unlock(thread);
        cond_wait(thread->t_wait);
        thread_lock(thread);
    }

    return 0;
}

int thread_wait(thread_t *thread, int reap, void **retval)
{
    int err = 0;
    (void)err;
    thread_assert_locked(thread);

    loop()
    {
        if (__thread_killed(current))
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

int thread_kill_all(void)
{
    current_assert();
    return tgroup_kill_thread(current->t_group, -1);
}

int thread_wake_n(thread_t *thread)
{
    int err = 0, held = 0;
    thread_assert_locked(thread);

    if (__thread_testflags(thread, THREAD_SETPARK))
        __thread_setflags(thread, THREAD_SETWAKE);

    if (!__thread_isleep(thread) || __thread_zombie(thread) || __thread_terminated(thread))
        return 0;

    if (thread->sleep_attr.queue == NULL)
        panic("%s:%d: No wait queue\n", __FILE__, __LINE__, thread->t_tid);

    if (thread->sleep_attr.guard)
    {
        if (!spin_locked(thread->sleep_attr.guard))
        {
            spin_lock(thread->sleep_attr.guard);
            held = 1;
        }
    }

    err = thread_remove_queue(thread, thread->sleep_attr.queue);

    if (thread->sleep_attr.guard)
    {
        if (spin_locked(thread->sleep_attr.guard) && held)
        {
            spin_unlock(thread->sleep_attr.guard);
            held = 0;
        }
    }

    if (err)
        return err;

    if (__thread_zombie(thread) || __thread_terminated(thread))
        return 0;

    if ((err = __thread_enter_state(thread, T_READY)))
        return err;

    return sched_park(thread);
}

int queue_get_thread(queue_t *queue, tid_t tid, thread_t **pthread)
{
    thread_t *thread = NULL;
    queue_node_t *next = NULL;

    if (!queue || !pthread)
        return -EINVAL;

    if (tid < 0)
        return -EINVAL;

    if (tid == thread_self())
    {
        *pthread = current;
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
            *pthread = thread;
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
        assert(!(err = tgroup_new(thread->t_tid, &kthreads)), "failed to create tgroup");
    }

    if ((err = thread_enqueue(kthreads->queue, thread, NULL)))
        goto error;

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

int kthread_create_join(void *(*entry)(void *), void *arg, void **ret)
{
    int err = 0;
    assert(entry, "no entry point");
    tid_t tid = 0;
    if ((err = kthread_create(entry, arg, &tid, NULL)))
        return err;
    return thread_join(tid, ret);
}

extern char __builtin_thread_entry, __builtin_thread_entry_end;
extern char __builtin_thread_arg, __builtin_thread_arg_end;

int start_builtin_threads(int *nthreads, thread_t ***threads)
{
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
    *threads = builtin_threads;
    return err;
}