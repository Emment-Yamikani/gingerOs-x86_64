#include <lib/printk.h>
#include <sys/sched.h>
#include <bits/errno.h>
#include <mm/kalloc.h>
#include <lib/string.h>
#include <ds/queue.h>
#include <sys/sched.h>
#include <sys/thread.h>
#include <arch/cpu.h>
#include <ginger/jiffies.h>
#include <arch/lapic.h>

static queue_t *embryo_queue = QUEUE_NEW("embryo-threads-queue");
static queue_t *zombie_queue = QUEUE_NEW("zombie-threads-queue");

int sched_init(void)
{
    int             err = 0;
    queue_t         *q = NULL;
    sched_queue_t   *sq = NULL;

    if (!(sq = kmalloc(sizeof *sq)))
        return -ENOMEM;

    memset(sq, 0, sizeof *sq);

    for (int i = 0; i < NLEVELS; ++i) {
        if ((err = queue_new("sched_queue", &q)))
            panic("error initailizing scheduler queues, error=%d\n", err);

        sq->level[i].queue = q;

        switch (i) {
        case 0:
            sq->level[i].quatum = 10; // 100ms
            break;
        case 1:
            sq->level[i].quatum = 15; // 150ms
            break;
        case 2:
            sq->level[i].quatum = 20; // 200ms
            break;
        case 3:
            sq->level[i].quatum = 25; // 250ms
            break;
        case 4:
            sq->level[i].quatum = 30; // 300ms
            break;
        case 5:
            sq->level[i].quatum = 35; // 350ms
            break;
        case 6:
            sq->level[i].quatum = 40; // 400ms
            break;
        case 7:
            sq->level[i].quatum = 50; // 500ms
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

    core = thread->t_sched_attr.processor;
    affinity = atomic_read(&thread->t_sched_attr.affinity);
    priority = atomic_read(&thread->t_sched_attr.priority);

    if (core == NULL)
        core = thread->t_sched_attr.processor = cpu;

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
    thread_enter_state(thread, T_ZOMBIE);

    /**
     * any zombie reaper must ensure thread
     * is in T_ZOMBIE state before freeing resources.
     * else thread must be placed back on the zombie queue.
    */
    if ((err = thread_enqueue(zombie_queue, thread, NULL)))
        return err;

    cond_broadcast(thread->t_wait);
    return 0;
}

int sched_sleep(queue_t *sleep_queue, spinlock_t *lock) {
    int err = 0;
    queue_assert(sleep_queue);
    current_assert_locked();

    if ((err = thread_enqueue(sleep_queue, current, &current->sleep_attr.node)))
        return err;

    current->sleep_attr.guard = lock;
    thread_enter_state(current, T_ISLEEP);
    current->sleep_attr.queue = sleep_queue;

    if (lock) spin_unlock(lock);

    sched();

    if (lock) spin_lock(lock);

    current->sleep_attr.node = NULL;
    current->sleep_attr.queue = NULL;

    if (thread_killed(current))
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
    thread_enter_state(thread, T_READY);

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
        assert(!thread_wake(thread), "failed to park thread");
        thread_unlock(thread);
    }

    queue_unlock(sleep_queue);
    return count;
}

void sched(void) {
    pushcli();
    uint64_t ncli = cpu->ncli;
    uint64_t intena = cpu->intena;
    
    //printk("%s:%d: %s() tid(%d), ncli: %d, intena: %d [%p]\n", __FILE__, __LINE__, __func__, current->t_tid, cpu->ncli, cpu->intena, return_address(0));
    
    current_assert_locked();
    
    if (current_issetpark() && current_isisleep()) {
        if (current_issetwake()) {
            current_mask_park_wake();
            popcli();
            return;
        }
    }
    
    swtch(&current->t_arch.t_ctx0, cpu->ctx);
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
    if ((core < 0) || (core > (cpu_count() - 1)))
        return -EINVAL;
    atomic_write(&thread->t_sched_attr.affinity, affinity);
    thread->t_sched_attr.processor = cpus[core];
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

void sched_self_destruct(void) {
    current_unlock();
    current_tgroup_lock();
    tgroup_dec_running(current_tgroup());
    current_tgroup_unlock();
    current_lock();

    if (current_isdetached()) {
        current_unlock();
        thread_detach(current);
        current_lock();
    }

    sched_zombie(current);
    current_unlock();
}

static jiffies_t next_time = 0;
static spinlock_t *next_timelk = &SPINLOCK_INIT();

jiffies_t nexttimer(void) {
    return next_time;
}

void sched_remove_zombies(void) {
    thread_t *thread = NULL;
    spin_lock(next_timelk);

    if (time_after(jiffies_get(), next_time)) {
        
        queue_lock(zombie_queue);
        
        if ((thread = thread_dequeue(zombie_queue))) {
            if (thread_isdetached(thread) && thread_iszombie(thread))
                thread_free(thread);
            else {
                thread_enqueue(zombie_queue, thread, NULL);
                thread_unlock(thread);
            }
        }
        
        queue_unlock(zombie_queue);

        next_time = s_TO_jiffies(600) + jiffies_get();
    }
    spin_unlock(next_timelk);
}

void schedule(void) {
    jiffies_t before = 0;
    thread_t *thread = NULL;

    sched_init();
    lapic_recalibrate(100);

    loop() {
        current = NULL;
        cpu->ncli = 0;
        cpu->intena = 0;

        sti();

        if (!(thread = sched_next()))
            continue;

        cli();

        if (thread_killed(thread)) {
            thread_enter_state(thread, T_TERMINATED);
            thread->t_exit = -EINTR;
            sched_self_destruct();
            continue;
        }

        current = thread;
        current_assert_locked();

        before = jiffies_get();
        current->t_sched_attr.last_sched = jiffies_TO_s(before);

        swtch(&cpu->ctx, current->t_arch.t_ctx0);
        
        current_assert_locked();
        current->t_sched_attr.cpu_time += (jiffies_get() - before);

        if (thread_killed(thread)) {
            thread_enter_state(thread, T_TERMINATED);
            thread->t_exit = -EINTR;
            sched_self_destruct();
            continue;
        }

        switch (current->t_state) {
        case T_EMBRYO:
            panic("??embryo was allowed to run\n");
            break;
        case T_TERMINATED:
            sched_self_destruct();
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

void sched_balancer(void) {
    BUILTIN_THREAD_ANOUNCE(__func__);
}

BUILTIN_THREAD(scheduler_balancer, sched_balancer, NULL);