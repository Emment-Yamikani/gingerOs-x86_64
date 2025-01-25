#include <sys/schedule.h>
#include <sys/thread.h>
#include <lib/string.h>
#include <lib/limits.h>
#include <core/debug.h>

const char *MLFQ_PRIORITY[] = {
    [MLFQ_LOWEST]       = "MLFQ_LOWEST",
    [MLFQ_LOWEST + 1]   = "MLFQ_MID - 1",
    [MLFQ_LOWEST + 2]   = "MLFQ_MID - 2",
    [MLFQ_HIGHEST]      = "MLFQ_HIGHEST",
    [NSCHED_LEVEL]      = NULL
};

#define level_lock(lvl)             ({ spin_lock(&(lvl)->lock); })
#define level_unlock(lvl)           ({ spin_unlock(&(lvl)->lock); })
#define level_islocked(lvl)         ({ spin_islocked(&(lvl)->lock); })
#define level_assert_locked(lvl)    ({ spin_assert_locked(&(lvl)->lock); })

static mlfq_t MLFQ[NCPU];

static QUEUE(terminated_threads);

static mlfq_t *mlfq_getself(void) {
    return &MLFQ[getcpuid()];
}

// per-cpu MLFQ init
static int mlfq_init(void) {
    usize   quant = 0;
    mlfq_t  *mlfq = mlfq_getself();

    memset(mlfq, 0, sizeof *mlfq);

    quant = ms_TO_jiffies(10); // quant/queue is a multiple of 10.
    for (int i = MLFQ_HIGHEST; i >= MLFQ_LOWEST; --i) {
        mlfq->level[i].lock    = SPINLOCK_INIT();
        mlfq->level[i].queue   = QUEUE_INIT();
        mlfq->level[i].quantum = quant;
        quant = quant + ms_TO_jiffies(10);
    }

    return 0;
}

int scheduler_init(void) {
    return mlfq_init();
}

static sched_level_t *mlfq_getlevel(mlfq_t *mlfq, int level) {
    if (!mlfq || level < MLFQ_LOWEST || level > MLFQ_HIGHEST)
        return NULL;
    return &mlfq->level[level];
}

static sched_level_t *mlfq_selflevel(int level) {
    return mlfq_getlevel(mlfq_getself(), level);
}

static queue_t *mlfq_selfqueue(int level) {
    sched_level_t *lev = mlfq_selflevel(level);
    return lev ? &lev->queue : NULL;
}

/**
 * @brief Attempts to steal threads from the most loaded CPU's MLFQ.
 *
 * This function identifies the most loaded CPU by summing the threads in all
 * priority levels of each CPU's MLFQ. It then attempts to steal threads from the
 * most loaded CPU, starting with the highest priority level.
 *
 * Locks are acquired for both the source and destination queues during the operation.
 *
 * @return void
 *
 * @note This function may fail to steal threads if no eligible threads are found.
 */
static void mlfq_steal(void) {
    int             err         = 0;
    int             max_load    = 0;            // Maximum thread count.
    sched_level_t   *src_level  = NULL;         // Source level pointer.
    queue_t         *dst_queue  = NULL;         // Destination queue.
    queue_t         *src_queue  = NULL;         // Source queue.
    mlfq_t          *most_loaded_mlfq = NULL;   // Pointer to the most loaded MLFQ.

    // Find the most loaded CPU's MLFQ.
    for (mlfq_t *mlfq = MLFQ; mlfq < &MLFQ[cpu_online()]; ++mlfq) {
        if (mlfq == mlfq_getself())
            continue;

        int total_threads = 0;
        for (int level = MLFQ_HIGHEST; level >= MLFQ_LOWEST; --level) {
            sched_level_t *level_ptr = mlfq_getlevel(mlfq, level);
            queue_lock(&level_ptr->queue);
            total_threads += queue_count(&level_ptr->queue);
            queue_unlock(&level_ptr->queue);
        }

        if (total_threads > max_load) {
            max_load = total_threads;
            most_loaded_mlfq = mlfq;
        }
    }

    // If no eligible CPU is found, return.
    if (!most_loaded_mlfq || max_load <= 2)
        return;

    // Attempt to steal threads from the most loaded CPU's MLFQ.
    for (int level = MLFQ_HIGHEST; level >= MLFQ_LOWEST; --level) {
        src_level = mlfq_getlevel(most_loaded_mlfq, level);
        level_lock(src_level);
        src_queue = &src_level->queue;
        queue_lock(src_queue);

        if (queue_count(src_queue) > 2) { // Ensure there are at least 2 threads in the source queue.
            level_lock(mlfq_selflevel(level));
            dst_queue = mlfq_selfqueue(level); // Get the self CPU's corresponding queue.
            queue_lock(dst_queue);

            // Migrate half the threads from the source queue to the destination queue.
            assert_eq(err = queue_node_migrate(
                dst_queue, src_queue,
                0,
                queue_count(src_queue) / 2, // Steal half the threads.
                QUEUE_RELLOC_HEAD
            ), 0, "Failed to reallocate nodes: %d\n", err);

            forlinked(node, dst_queue->head, node->next) {
                thread_t *thread = (thread_t *)node->data;

                thread_lock(thread);
                queue_lock(&thread->t_queues);

                // replace current queue with the next holding queue.
                assert_eq(err = queue_replace(&thread->t_queues, src_queue,
                    dst_queue), 0,
                    "Failed to replace queue for thread[%d:%d], error: %d\n",
                    thread_getpid(thread), thread_gettid(thread), err
                );

                queue_unlock(&thread->t_queues);
                thread_unlock(thread);
            }

            queue_unlock(dst_queue);
            level_unlock(mlfq_selflevel(level));
        }

        queue_unlock(src_queue);
        level_unlock(src_level);
    }
}

static thread_t *mlfq_getnext(void) {
    thread_t    *thread = NULL;
    queue_t     *queue  = NULL;

    for (int level = MLFQ_HIGHEST; level >= MLFQ_LOWEST; --level) {
        queue = mlfq_selfqueue(level);
        queue_lock(queue);
        if ((thread = thread_dequeue(queue))) {
            queue_unlock(queue);
            return thread;
        }
        queue_unlock(queue);
    }

    return NULL;
}

void mlfq_priority_boost(void) {
    int     err = 0;
    thread_t *thread = NULL;
    queue_t  *queue = NULL;

    for (int level = MLFQ_HIGHEST - 1; level >= MLFQ_LOWEST; --level) {
        queue = mlfq_selfqueue(level);
        queue_lock(queue);

        // increment the priority of threads on this queue.        
        forlinked(node, queue->head, node->next) {
            thread = (thread_t *)node->data; // get the thread pointer.

            thread_lock(thread);
            thread->t_sched.ts_priority++; // boost thread's priority.
            // ensure thread now has the timeslice of the new priority level.
            thread->t_sched.ts_timeslice = mlfq_selflevel(level + 1)->quantum;
            
            queue_lock(&thread->t_queues);

            // replace current queue for the next holding queue.
            assert_eq(err = queue_replace(&thread->t_queues, queue,
                mlfq_selfqueue(level + 1)), 0,
                "Failed to replace queue for thread[%d:%d], error: %d\n",
                thread_getpid(thread), thread_gettid(thread), err
            );

            queue_unlock(&thread->t_queues);

            thread_unlock(thread);
        }

        queue_lock(mlfq_selfqueue(level + 1)); // lock the destination.
        // move from lower priority queue to a higher one.
        queue_move(mlfq_selfqueue(level + 1), queue, QUEUE_RELLOC_TAIL);
        queue_unlock(mlfq_selfqueue(level + 1)); // unlock the destination.
        
        queue_unlock(queue);
    }
}

void mlfq_adjust_quantum(void) {
    mlfq_t *mlfq = mlfq_getself();
    int total_threads = 0;

    // Calculate total threads in all levels
    for (int i = MLFQ_HIGHEST; i >= MLFQ_LOWEST; --i) {
        queue_lock(&mlfq->level[i].queue);
        total_threads += queue_count(&mlfq->level[i].queue);
        queue_unlock(&mlfq->level[i].queue);
    }

    for (int i = MLFQ_HIGHEST; i >= MLFQ_LOWEST; --i) {
        sched_level_t *level = &mlfq->level[i];
        level_lock(level);

        queue_lock(&level->queue);
        int queue_size = queue_count(&level->queue);
        queue_unlock(&level->queue);

        // Adjust quantum based on queue size relative to total threads
        if (queue_size > total_threads / 2) {
            level->quantum += ms_TO_jiffies(5); // Increase by 5ms
        } else if (queue_size < total_threads / 4 && level->quantum > ms_TO_jiffies(10)) {
            level->quantum -= ms_TO_jiffies(5); // Decrease by 5ms (min 10ms)
        }
        level_unlock(level);
    }
}

int load_balance_threshold(void) {
    int total_load = 0;

    for (int cpu_i = 0; cpu_i < cpu_online(); ++cpu_i) {
        mlfq_t *mlfq = &MLFQ[cpu_i];
        for (int level = MLFQ_HIGHEST; level >= MLFQ_LOWEST; --level) {
            queue_lock(&mlfq->level[level].queue);
            total_load += queue_count(&mlfq->level[level].queue);
            queue_unlock(&mlfq->level[level].queue);
        }
    }

    return total_load / (2 * cpu_online()); // Set threshold to 50% of the average load per CPU
}

void mlfq_load_balance(void) {
    int err = 0, max_load = 0, min_load = INT_MAX;
    mlfq_t *most_loaded = NULL, *least_loaded = NULL;

    // Find the most loaded and least loaded CPUs
    for (int cpu_i = 0; cpu_i < cpu_online(); ++cpu_i) {
        mlfq_t *mlfq = &MLFQ[cpu_i];
        int load = 0;

        for (int level = MLFQ_HIGHEST; level >= MLFQ_LOWEST; --level) {
            queue_lock(&mlfq->level[level].queue);
            load += queue_count(&mlfq->level[level].queue);
            queue_unlock(&mlfq->level[level].queue);
        }

        if (load > max_load) {
            max_load = load;
            most_loaded = mlfq;
        }
        if (load < min_load) {
            min_load = load;
            least_loaded = mlfq;
        }
    }

    // Migrate threads if there's a significant imbalance
    if (most_loaded && least_loaded && (max_load - min_load > load_balance_threshold())) {
        for (int level = MLFQ_HIGHEST; level >= MLFQ_LOWEST; --level) {
            sched_level_t *src_level = mlfq_getlevel(most_loaded, level);
            sched_level_t *dst_level = mlfq_getlevel(least_loaded, level);

            if (!src_level || !dst_level) continue;

            level_lock(src_level);
            level_lock(dst_level);

            queue_t *src_queue = &src_level->queue;
            queue_t *dst_queue = &dst_level->queue;

            queue_lock(src_queue);
            int threads_to_move = queue_count(src_queue) / 4; // Move 25% of threads


            if (threads_to_move > 0) {
                queue_lock(dst_queue);
                assert_eq(err = queue_node_migrate(
                    dst_queue, src_queue,
                    0, threads_to_move, QUEUE_RELLOC_HEAD
                ), 0, "Failed to migrate threads, error: %d\n", err);

                queue_unlock(dst_queue);

                forlinked(node, dst_queue->head, node->next) {
                    thread_t *thread = (thread_t *)node->data;

                    thread_lock(thread);
                    queue_lock(&thread->t_queues);

                    // replace current queue with the next holding queue.
                    assert_eq(err = queue_replace(&thread->t_queues, src_queue,
                        dst_queue), 0,
                        "Failed to replace queue for thread[%d:%d], error: %d\n",
                        thread_getpid(thread), thread_gettid(thread), err
                    );

                    queue_unlock(&thread->t_queues);
                    thread_unlock(thread);
                }
            }

            queue_unlock(src_queue);
            level_unlock(dst_level);
            level_unlock(src_level);
        }
    }
}

void scheduler_tick(void) {
    static usize ticks = 0;
    ticks++;

    if (ticks % (100 * 3) == 0) {
        mlfq_priority_boost();
    }

    if (ticks % (100 * 10) == 0) {
        mlfq_load_balance();
    }

    if (ticks % 100 == 0) {
        mlfq_adjust_quantum();
    }
}

int mlfq_enqueue(thread_t *thread) {
    int     err     = 0;
    int     level   = 0;
    queue_t *queue  = NULL;

    thread_assert_locked(thread);

    level = thread->t_sched.ts_priority;

    queue = mlfq_selfqueue(level);

    if ((err = thread_enqueue(queue, thread, NULL)))
        return err;

    thread->t_sched.ts_timeslice= mlfq_selflevel(level)->quantum;
    thread_enter_state(thread, T_READY);

    return 0;
}

void sched(void) {
    long ncli = 0, intena = 0;

    pushcli();
    ncli    = cpu->ncli;
    intena  = cpu->intena;
    current_assert_locked();

    if (current_issetpark() && current_isisleep()) {
        if (current_issetwake()) {
            current_mask_park_wake();
            popcli();
            return;
        }
    }

    // thread has used up it's time allotment.
    if (current->t_sched.ts_timeslice == 0) {
        // step it down 1 priority level.
        if (current->t_sched.ts_priority)
            current->t_sched.ts_priority--;
    }

    context_switch(&current->t_arch.t_ctx);

    current_assert_locked();
    cpu->ncli   = ncli;
    cpu->intena = intena;
    popcli();
}

void sched_yield(void) {
    current_lock();
    current_enter_state(T_READY);
    sched();
    current_unlock();
}

int sched_park(thread_t *thread) {
    thread_assert_locked(thread);
    thread->t_sched.ts_priority = MLFQ_HIGHEST;
    return mlfq_enqueue(thread);
}

static void sched_self_terminate(void) {
    int err = 0;

    current_assert_locked();

    pushcli(); // prevent enabling interrupts

    assert_eq(current_getstate(), T_TERMINATED,
        "thread[%d:%d] state[%s]: must be terminated\n",
        getpid(), gettid(), t_states[current_getstate()]
    );

    assert_eq(err = thread_enqueue(terminated_threads, current, NULL), 0,
        "thread[%d:%d]: failed to enqueue, error: %d\n", getpid(), gettid(), err
    );

    current_unlock();
    debug("cpu[%d] thread[%d:%d]: terminated\n", getcpuid(), getpid(), gettid());
}

/**
 * @brief Handles the current thread's state and performs the necessary actions.
 * 
 * This function checks the state of the current thread and handles it based on
 * its value, such as enqueuing, unlocking, or terminating the thread.
 *
 * @param current The current thread being handled.
 * @return void
 */
static void handle_thread_state(void) {
    int err = 0;

    current_assert_locked();

    if (current_iskilled()) {
        current_enter_state(T_TERMINATED);
        current->t_exit = -EINTR;
    }

    switch (current_getstate()) {
    case T_RUNNING:
    case T_READY:
        assert_eq(err = mlfq_enqueue(current), 0,
            "Failed to enqueue thread[%d:%d], error: %d\n",
            getpid(), gettid(), err
        );
        current_unlock();
        break;
    case T_ISLEEP:
    case T_USLEEP:
    case T_STOPPED:
        current_unlock();
        break;
    case T_TERMINATED:
        sched_self_terminate();
        break;
    default:
        assert(0, "cpu[%d] thread[%d:%d] state[%s]: Returned to scheduler()\n",
            getcpuid(), getpid(), gettid(), t_states[current->t_state]
        );
    }
}

__noreturn void scheduler(void) {
    int             err     = 0;
    uintptr_t       pdbr    = 0;
    jiffies_t       before  = 0;
    mmap_t          *mmap   = NULL;
    thread_sched_t  *tsched = NULL;
    arch_thread_t   *tarch  = NULL;

    loop() {
        cpu->ncli   = 0;
        cpu->intena = 0;
        current     = NULL;

        sti();

        if (NULL == (current = mlfq_getnext())) {
            mlfq_steal(); // attempt stealing some threads
            if (NULL == (current = mlfq_getnext())) {
                hlt();
                continue;
            }
        }

        /// ensure current thread is locked,
        /// this will be implicity unlocked,
        /// in functions like arch_thread_start().
        current_assert_locked();

        if (current_iskilled()) {
            current_enter_state(T_ZOMBIE);
            current->t_exit = -EINTR;
        }

        switch (current_getstate()) {
        case T_READY:
            current_enter_state(T_RUNNING);
        case T_RUNNING:
        case T_ZOMBIE: // allow zombie to run to clean up
            break;
        case T_TERMINATED:
            sched_self_terminate();
            continue;
        default:
            assert(0, "cpu[%d] thread[%d:%d] state[%s]: scheduled to run??\n",
                getcpuid(), getpid(), gettid(), t_states[current->t_state]
            );
        }

        mmap    = current->t_mmap;
        tarch   = &current->t_arch;
        tsched  = &current->t_sched;

        // get the current time of scheduling.
        tsched->ts_last_sched = jiffies_TO_s(before = jiffies_get());

        if (mmap) {
            mmap_lock(mmap);
            mmap_focus(mmap, &pdbr);
            mmap_unlock(mmap);
            // Make sure a thread running in a seperate address space
            // to that of the kernel must have it's kernel stack pointer
            // set up in the tss.
            // TODO: use tss.ist in later versions of this code.
            err = arch_thread_setkstack(tarch);
            assert(err == 0, "Kernel stack was not set for user thread, errno = %d\n", err);
        } else arch_swtchvm(0, NULL);

        context_switch(&tarch->t_ctx);

        // enssure thread was locked before returning
        current_assert_locked();

        // get the time thread returned execution to the scheduler.
        tsched->ts_cpu_time += jiffies_TO_s(jiffies_get() - before);

        pushcli(); // Ensure atomicity.

        handle_thread_state();
    }
}

__noreturn void scheduler_load_balance(void) {
    loop() {
        jiffies_sleep(s_TO_jiffies(10));
        mlfq_load_balance();
    }
} BUILTIN_THREAD(load_balance, scheduler_load_balance, NULL);
