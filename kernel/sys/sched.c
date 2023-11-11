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

static jiffies_t next_time = 0;
static spinlock_t *next_timelk = &SPINLOCK_INIT();
queue_t *sched_stopq = QUEUE_NEW(/*"stopped-queue"*/);
static queue_t *embryo_queue = QUEUE_NEW(/*"embryo-threads-queue"*/);
static queue_t *zombie_queue = QUEUE_NEW(/*"zombie-threads-queue"*/);

jiffies_t nexttimer(void) {
    return next_time;
}

void sched_remove_zombies(void) {
    thread_t *thread = NULL;
    spin_lock(next_timelk);

    if (time_after(jiffies_get(), next_time)) {
        
        queue_lock(zombie_queue);
        thread = thread_dequeue(zombie_queue);
        queue_unlock(zombie_queue);
        if (thread) {
            if (thread_isdetached(thread) && thread_iszombie(thread))
                thread_free(thread);
            else {
                thread_enqueue(zombie_queue, thread, NULL);
                thread_unlock(thread);
            }
        }
        
        next_time = s_TO_jiffies(600) + jiffies_get();
    }
    spin_unlock(next_timelk);
}

int sched_init(void) {
    int err = 0;

    cpu->ncli = 0;
    current = NULL;
    cpu->intena = 0;
    
    memset(&ready_queue, 0, sizeof ready_queue);

    for (size_t i = 0; i < NELEM(ready_queue.level); ++i) {
        if ((err = queue_alloc(&ready_queue.level[i].queue)))
            goto error;
        ready_queue.level[i].quatum = i * 5 + 10;
    }

    return 0;
error:
    for (size_t i = 0; i < NELEM(ready_queue.level); ++i) {
        if (ready_queue.level[i].queue) {
            queue_free(ready_queue.level[i].queue);
            ready_queue.level->queue = NULL;
        }
    }
    return err;
}

void sched(void) {
    long ncli = 0, intena = 0;

    pushcli();
    ncli = cpu->ncli;
    intena = cpu->intena;
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
    cpu->intena = intena;
    cpu->ncli = ncli;
    popcli();
}

int sched_zombie(thread_t *thread) {
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

    cond_broadcast(&thread->t_wait);
    return 0;
}

int sched_sleep(queue_t *sleep_queue, tstate_t state, spinlock_t *lock) {
    int err = 0;
    queue_assert(sleep_queue);
    current_assert_locked();

    if ((err = thread_enqueue(sleep_queue, current, &current->sleep_attr.node)))
        return err;

    current->sleep_attr.guard = lock;
    thread_enter_state(current, state);
    current->sleep_attr.queue = sleep_queue;

    if (lock) spin_unlock(lock);

    sched();

    if (lock) spin_lock(lock);

    current->sleep_attr.node = NULL;
    current->sleep_attr.queue = NULL;

    if (thread_iskilled(current))
        return -EINTR;

    return 0;
}

int sched_wake1(queue_t *sleep_queue) {
    tstate_t state = 0;
    int retval = -ESRCH;
    thread_t *thread = NULL;

    queue_lock(sleep_queue);
    if ((thread = thread_dequeue(sleep_queue))) {
        thread_assert_locked(thread);
        state = thread_getstate(thread);
        thread_enter_state(thread, T_READY);
        if (sched_park(thread)) {
            thread_enter_state(thread, state);
            thread_enqueue(sleep_queue, thread, &thread->sleep_attr.node);
        }
        thread_unlock(thread);
        retval = 0;
    }
    queue_unlock(sleep_queue);

    return retval;
}

size_t sched_wakeall(queue_t *sleep_queue) {
    int err = 0;
    size_t count = 0;
    thread_t *thread = NULL;
    queue_node_t *next = NULL;

    queue_lock(sleep_queue);
    count = queue_count(sleep_queue);

    forlinked(node, sleep_queue->head, next) {
        next = node->next;
        thread = node->data;
        thread_lock(thread);
        if ((err = thread_wake(thread)))
            panic("failed to park thread[%d], err=%d\n", thread_gettid(thread), err);
        thread_unlock(thread);
    }

    queue_unlock(sleep_queue);
    return count;
}

void sched_yield(void) {
    current_lock();
    current->t_state = T_READY;
    sched();
    current_unlock();
}

void sched_self_destruct(void) {
    int err = 0;
    /**
     * pushcli() to avoid interrupts being enabled
     * immediately we do current_unlock() at the end of this routine.
     * because that will call reentrance into schedule() from trap().
     * TRUST ME: it's a VERY NASTY BUG :).
    */
    pushcli();
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

    if ((err = sched_zombie(current)))
        panic("FAILED TOO ZOMBIE: err = %d\n", err);
    if (current->t_arch.t_sig_kstack)
        thread_free_kstack(current->t_arch.t_sig_kstack, current->t_arch.t_sig_kstacksz);
    current_unlock();
}

int sched_park(thread_t *thread) {
    int err = 0;
    long prio = 0;
    cpu_t *proc = NULL;
    level_t *level = NULL;

    thread_assert_locked(thread);

    if (thread == NULL)
        return -EINVAL;

    if (thread_isembryo(thread))
        return thread_enqueue(embryo_queue, thread, NULL);

    proc = thread->t_sched_attr.processor;
    prio = thread->t_sched_attr.priority;
    level = &ready_queue.level[SCHED_LEVEL(prio)];

    if (proc == NULL) {
        proc = cpu;
        thread->t_sched_attr.processor = proc;
    }

    if ((err = thread_enqueue(level->queue, thread, NULL)))
        goto error;

    thread->t_sched_attr.timeslice = level->quatum;
    return 0;
error:
    return err;
}

thread_t *sched_next(void) {
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
    for (int i = 0; i < NLEVELS; ++i) {
        level = &ready_queue.level[i];
        queue_lock(level->queue);
        if (!(thread = thread_dequeue(level->queue))) {
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

__noreturn void schedule(void) {
    int err = 0;
    jiffies_t before = 0;
    thread_t *thread = NULL;

    if ((err = sched_init()))
        panic("CPU%d: failed to initialize scheduler queues\n");

    lapic_recalibrate(SYS_HZ);

    loop() {
        cpu->ncli = 0;
        current = NULL;
        cpu->intena = 0;

        sti();

        if (!(thread = sched_next()))
            continue;

        cli();

        current = thread;
        
        current_assert_locked();

        if (thread_iskilled(thread) ||
            thread_iszombie(thread) ||
            thread_isterminated(thread)) {
            thread_enter_state(thread, T_TERMINATED);
            thread->t_exit = -EINTR;
            sched_self_destruct();
            continue;
        }

        if (current_isstopped()) {
            pushcli();
            thread_stop(current, sched_stopq);
            continue;
        }

        before = jiffies_get();
        current->t_sched_attr.last_sched = jiffies_TO_s(before);

        swtch(&cpu->ctx, current->t_arch.t_ctx0);
        
        pushcli();
        current_assert_locked();
        current->t_sched_attr.cpu_time += (jiffies_get() - before);

        if (thread_iskilled(thread)) {
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
        case T_USLEEP:
        case T_STOPPED:
        case T_ISLEEP:
            current_unlock();
            break;
        default:
            panic("tid: %d state: %s\n", thread_self(), t_states[current->t_state]);
        }
    }
}