#include <sys/sched.h>
#include <sys/thread.h>
#include <lib/stddef.h>
#include <lib/stdint.h>
#include <bits/errno.h>

queue_t *sched_stopq            = QUEUE_NEW();  /*"stopped-queue"*/
static queue_t *embryo_queue    = QUEUE_NEW();  /*"embryo-threads-queue"*/
static queue_t *zombie_queue    = QUEUE_NEW();  /*"zombie-threads-queue"*/

int sched_sleep(queue_t *sleep_queue, tstate_t state, spinlock_t *lock) {
    int err = 0;

    if (sleep_queue == NULL)
        return -EINVAL;

    queue_assert(sleep_queue);
    current_assert_locked();

    if ((err = thread_enqueue(sleep_queue, current, &current->t_sleep.node)))
        return err;

    current_enter_state(state);
    current->t_sleep.guard = lock;
    current->t_sleep.queue = sleep_queue;

    if (lock) spin_unlock(lock);

    sched();

    if (lock) spin_lock(lock);

    current->t_sleep.node   = NULL;
    current->t_sleep.queue  = NULL;
    current->t_sleep.guard  = NULL;

    if (current_iskilled())
        return -EINTR;

    return 0;
}

int sched_sleep_r(queue_t *sleep_queue, tstate_t state, spinlock_t *lock) {
    int     err = 0;

    if (sleep_queue == NULL)
        return -EINVAL;

    current_assert_locked();
    /**
     * @brief Rellocate the thread to the head of the tgroup queue.
     * @FIXME: Do we need to do this for all kinds of sleep states,
     * or do we only need it for interruptable sleep states?
     */
    if ((err = queue_rellocate_node(current->t_tgroup, current->t_tgrp_qn, QUEUE_RELLOC_HEAD)))
        return err;

    if ((err = thread_enqueue(sleep_queue, current, &current->t_sleep.node)))
        return err;

    current_enter_state(state);
    current->t_sleep.guard = lock;
    current->t_sleep.queue = sleep_queue;

    if (lock != NULL)
        spin_unlock(lock);

    queue_unlock(current->t_tgroup);
    
    sched();    // jmp back to the scheduler.
    
    /**
     * @brief unlock here inorder to respect the
     * loc ordering of the func-stack call chain.
     *  lock ? before tgroup ? before current.
     * SEE: lines below.
     */
    current_unlock(); // FIXME: ???

    if (lock != NULL)
        spin_lock(lock);
    
    queue_lock(current->t_tgroup);
    current_lock();

    current->t_sleep.node  = NULL;
    current->t_sleep.queue = NULL;
    current->t_sleep.guard = NULL;

    return current_iskilled() ? -EINTR : 0;
}

int sched_wake1(queue_t *sleep_queue) {
    tstate_t    state   = 0;
    thread_t    *thread = NULL;
    int         retval  = -ESRCH;

    queue_lock(sleep_queue);
    if ((thread = thread_dequeue(sleep_queue))) {
        thread_assert_locked(thread);
        state = thread_getstate(thread);
        thread_enter_state(thread, T_READY);
        if (sched_park(thread)) {
            thread_enter_state(thread, state);
            thread_enqueue(sleep_queue, thread, &thread->t_sleep.node);
        }
        thread_unlock(thread);
        retval = 0;
    }
    queue_unlock(sleep_queue);

    return retval;
}

size_t sched_wakeall(queue_t *sleep_queue) {
    int             err     = 0;
    size_t          count   = 0;
    queue_node_t    *next   = NULL;
    thread_t        *thread = NULL;

    queue_lock(sleep_queue);
    count = queue_count(sleep_queue);

    forlinked(node, sleep_queue->head, next) {
        next    = node->next;
        thread  = node->data;
        thread_lock(thread);
        if ((err = thread_wake(thread)))
            panic("failed to wake thread[%d], err=%d\n", thread_gettid(thread), err);
        thread_unlock(thread);
    }

    queue_unlock(sleep_queue);
    return count;
}

thread_t *sched_getzombie(void) {
    thread_t *zombie = NULL;
    queue_lock(zombie_queue);
    zombie = thread_dequeue(zombie_queue);
    queue_unlock(zombie_queue);
    return zombie;
}

int sched_putzombie(thread_t *thread) {
    int err = 0;

    if (thread == NULL)
        return -EINVAL;
    
    thread_assert_locked(thread);
    thread_enter_state(thread, T_ZOMBIE);

    if ((err = thread_enqueue(zombie_queue, thread, NULL)))
        return err;
    
    cond_broadcast(&thread->t_wait);
    return 0;
}

thread_t *sched_getembryo(void) {
    thread_t *embryo = NULL;
    queue_lock(embryo_queue);
    embryo = thread_dequeue(embryo_queue);
    queue_unlock(embryo_queue);
    return embryo;
}

int sched_putembryo(thread_t *thread) {
    if (thread == NULL)
        return -EINVAL;

    thread_assert_locked(thread);
    thread_enter_state(thread, T_EMBRYO);
    return thread_enqueue(embryo_queue, thread, NULL);
}

void sched_remove_zombies(void) {
    thread_t *zombie = NULL;

    if ((atomic_read(&cpu->timer_ticks) % (size_t)s_TO_jiffies(300)))
        return;

    if ((zombie = sched_getzombie())) {
        if (thread_iszombie(zombie) && thread_isdetached(zombie))
            thread_free(zombie);
        else {
            sched_putzombie(zombie);
            thread_unlock(zombie);
        }
    }
}
