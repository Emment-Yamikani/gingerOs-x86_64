#include <lib/printk.h>
#include <sys/sched.h>
#include <bits/errno.h>
#include <mm/kalloc.h>
#include <lib/string.h>
#include <ds/queue.h>
#include <sys/sched.h>
#include <sys/thread.h>
#include <arch/cpu.h>

static queue_t *embryo_queue = NULL;
static queue_t *zombie_queue = NULL;

int init_sched_queues(void) {
    int err = 0;
    if ((err = queue_new("embryo-threads", &embryo_queue)))
        return err;

    if ((err = queue_new("zombie-threads", &zombie_queue)))
        return err;
    return 0;
}

int sched_init(void)
{
    int err = 0;
    sched_queue_t *sq = NULL;
    queue_t *q = NULL;

    if (!(sq = kmalloc(sizeof *sq)))
        return -ENOMEM;

    memset(sq, 0, sizeof *sq);

    for (int i = 0; i < NLEVELS; ++i)
    {
        if ((err = queue_new("sched_queue", &q)))
            panic("error initailizing scheduler queues, error=%d\n", err);

        sq->level[i].queue = q;

        switch (i)
        {
        case 0:
            sq->level[i].quatum = 50;
            break;
        case 1:
            sq->level[i].quatum = 75;
            break;
        case 2:
            sq->level[i].quatum = 100;
            break;
        case 3:
            sq->level[i].quatum = 125;
            break;
        case 4:
            sq->level[i].quatum = 150;
            break;
        case 5:
            sq->level[i].quatum = 175;
            break;
        case 6:
            sq->level[i].quatum = 200;
            break;
        case 7:
            sq->level[i].quatum = 250;
            break;
        }
    }

    current = NULL;
    cpu->ncli = 0;
    cpu->intena = 0;
    ready_queue = sq;
    return 0;
}

int sched_park(thread_t *thread)
{
    int err = 0;
    long priority = 0;
    long affinity = 0;
    cpu_t *core = NULL;
    level_t *level = NULL;

    thread_assert_locked(thread);

    if (thread->t_state == T_EMBRYO)
        return thread_enqueue(embryo_queue, thread, NULL);

    core = thread->t_sched_attr.core;
    affinity = atomic_read(&thread->t_sched_attr.affinity);
    priority = atomic_read(&thread->t_sched_attr.priority);

    if (core == NULL)
        core = thread->t_sched_attr.core = cpu;

    switch (affinity)
    {
    case SCHED_SOFT_AFFINITY:
        level = &core->queueq->level[SCHED_LEVEL(priority)];
        break;
    case SCHED_HARD_AFFINITY:
        level = &core->queueq->level[SCHED_LEVEL(priority)];
        break;
    default:
        panic("Invalid affinity attribute\n");
    }

    atomic_write(&thread->t_sched_attr.timeslice, level->quatum);
    if ((err = thread_enqueue(level->queue, thread, NULL)))
        goto error;
    return 0;
error:
    printk("\e[0;4mfailed to put on ready queue\e[0m\n");
    return err;
}

thread_t *sched_next(void)
{
    level_t *level = NULL;
    thread_t *thread = NULL;

    queue_lock(embryo_queue);
    thread = thread_dequeue(embryo_queue);
    queue_unlock(embryo_queue);

    if (thread == NULL)
        goto self;

    thread_assert_locked(thread);
    thread->t_state = T_READY;
    assert(!sched_park(thread), "failed to park thread");
    thread_unlock(thread); // release thread

self:
    thread = NULL;
    pushcli();
    for (int i = 0; i < NLEVELS; ++i)
    {
        level = &ready_queue->level[i];
        queue_lock(level->queue);
        if (!(thread = thread_dequeue(level->queue)))
        {
            queue_unlock(level->queue);
            continue;
        }
        thread->t_state = T_RUNNING;
        atomic_write(&thread->t_sched_attr.timeslice, level->quatum);
        queue_unlock(level->queue);
        break;
    }
    popcli();
    return thread;
}

int sched_zombie(thread_t *thread)
{
    int err = 0;
    thread_assert_locked(thread);

    if ((err = thread_enqueue(zombie_queue, thread, NULL)))
        return err;

    atomic_dec(&thread->t_group->nthreads);
    cond_broadcast(thread->t_wait);
    return 0;
}

int sched_sleep(queue_t *sleep_queue, spinlock_t *lock)
{
    int err = 0;
    queue_assert(sleep_queue);
    current_assert_locked();

    if ((err = thread_enqueue(sleep_queue, current, &current->sleep_attr.node)))
        return err;

    current->sleep_attr.guard = lock;
    __thread_enter_state(current, T_ISLEEP);
    current->sleep_attr.queue = sleep_queue;

    // printk("sleep_node: %p, sleep_node->next: %p\n", current->t_sleep_node, current->t_sleep_node->next);
    if (lock)
        spin_unlock(lock);

    sched();

    if (lock)
        spin_lock(lock);

    current->sleep_attr.node = NULL;
    current->sleep_attr.queue = NULL;

    if (__thread_killed(current))
        return -EINTR;

    return 0;
}

int sched_wake1(queue_t *sleep_queue)
{
    int woken = 0;
    thread_t *thread = NULL;

    queue_lock(sleep_queue);
    thread = thread_dequeue(sleep_queue);
    queue_unlock(sleep_queue);

    if (thread == NULL)
        return 0;

    thread_assert_locked(thread);
    __thread_enter_state(thread, T_READY);

    sched_park(thread);
    thread_unlock(thread);
    return woken;
}

int sched_wakeall(queue_t *sleep_queue)
{
    int count = 0;
    thread_t *thread = NULL;
    queue_node_t *next = NULL;

    queue_lock(sleep_queue);
    count = queue_count(sleep_queue);

    forlinked(node, sleep_queue->head, next)
    {
        next = node->next;
        thread = node->data;
        thread_lock(thread);
        assert(!thread_wake_n(thread), "failed to park thread");
        thread_unlock(thread);
    }

    queue_unlock(sleep_queue);
    return count;
}

void sched(void)
{
    pushcli();
    int ncli = cpu->ncli;
    int intena = cpu->intena;
    
    //printk("%s:%d: %s() tid(%d), ncli: %d, intena: %d [%p]\n", __FILE__, __LINE__, __func__, current->t_tid, cpu->ncli, cpu->intena, return_address(0));
    
    current_assert_locked();

    swtch(&current->t_arch.t_ctx, cpu->ctx);
    current_assert_locked();
    
    //printk("%s:%d: %s() tid(%d), ncli: %d, intena: %d [%p]\n", __FILE__, __LINE__, __func__, current->t_tid, cpu->ncli, cpu->intena, return_address(0));
    
    cpu->ncli = ncli;
    cpu->intena = intena;
    popcli();
}

int sched_setattr(thread_t *thread, int affinity, int core)
{
    thread_assert_locked(thread);

    if ((affinity < SCHED_SOFT_AFFINITY) || (affinity > SCHED_HARD_AFFINITY))
        return -EINVAL;
    if ((core < 0) || (core > (get_cpu_count() - 1)))
        return -EINVAL;
    atomic_write(&thread->t_sched_attr.affinity, affinity);
    thread->t_sched_attr.core = cpus[core];
    return 0;
}

void sched_set_priority(thread_t *thread, int priority)
{
    thread_assert_locked(thread);
    if (priority < 0 || priority > 255)
        priority = SCHED_LOWEST_PRIORITY;
    atomic_write(&thread->t_sched_attr.priority, priority);
}

void sched_yield(void)
{
    current_lock();
    current->t_state = T_READY;
    sched();
    current_unlock();
}

void schedule(void)
{
    thread_t *thread = NULL;
    sched_init();

    for (;;)
    {
        current = NULL;

        cpu->ncli = 0;
        cpu->intena = 0;

        sti();

        if (!(thread = sched_next()))
            continue;

        cli();

        if (__thread_killed(thread))
        {
            thread->t_state = T_ZOMBIE;
            thread->t_exit = -EINTR;
            sched_zombie(thread);
            thread_unlock(thread);
            continue;
        }

        current = thread;

        current_assert_locked();
        swtch(&cpu->ctx, current->t_arch.t_ctx);
        current_assert_locked();

        // paging_switch(oldpgdir);

        switch (current->t_state)
        {
        case T_EMBRYO:
            panic("embryo was allowed to run\n");
            break;
        case T_TERMINATED:
            __fallthrough;
        case T_ZOMBIE:
            assert(!sched_zombie(current), "couldn't zombie");
            current_unlock();
            break;
        case T_READY:
            sched_park(current);
            current_unlock();
            break;
        case T_RUNNING:
            panic("thread sched while running???\n");
            break;
        case T_ISLEEP:
            current_unlock();
            break;
        default:
            panic("tid: %d state: %d\n", current->t_tid, current->t_state);
        }
    }
}

#include <boot/boot.h>

void sched_balancer(void) {
    BUILTIN_THREAD_ANOUNCE(__func__);
}

BUILTIN_THREAD(scheduler_balancer, sched_balancer, NULL);