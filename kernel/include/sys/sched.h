#pragma once

#include <sync/atomic.h>
#include <lib/stdint.h>
#include <lib/stddef.h>
#include <lib/types.h>

extern void sched(void);
extern int sched_init(void);

#define NLEVELS 8
#define SCHED_HIGHEST_PRIORITY  0
#define SCHED_LOWEST_PRIORITY 255

#define SCHED_LEVEL(p) ((p) / ((SCHED_LOWEST_PRIORITY + 1) / NLEVELS))

typedef struct level {
    atomic_t    pull;   // pull flag
    long        quatum; // quatum
    queue_t     *queue; // queue
} level_t;

typedef struct sched_queue {
    level_t level[NLEVELS];
} sched_queue_t;

typedef struct sched_t {
    char *s_name;
    enum {
        SCHED_RR,
        SCHED_MLFQ,
        SCHED_FIFO,
        SCHED_LOTT,
    }   s_type;
    int (*s_init)();
    int (*s_park)();
    int (*s_next)();
} sched_t;

extern queue_t *sched_stopq;

/*queue up a thread*/
int sched_park(thread_t *);

/**
 * @brief give up the cpu for one scheduling round
 *
 */
void sched_yield(void);

/**
 * @brief put thread on the zombie queue.
 * then signal(wake up) all threads waiting for a signal from this thread.
 * caller must hold thread->t_lock.
 *
 * @param thread
 * @return int
 */
int sched_putzombie(thread_t *thread);
thread_t *sched_getzombie(void);
void sched_remove_zombies(void);

/**
 * @brief wake one 'thread' from 'sleep_queue'.
 *
 * @param sleep_queue
 */
int sched_wake1(queue_t *sleep_queue);

/**
 * @brief causes the current 'thread' to sleep on 'sleep_queue'.
 * Also releases 'lock' if specified
 * and switches to the scheduler while holding current->t_lock
 * @param sleep_queue sleep queue on which to place 'current'
 * @param state state in which we want the thread(current) to be.
 * @param lock  hand-over-lock passed to current upon waking up the thread.
 * @return int  0 on success otherwise an error code is returned.
 */
int sched_sleep(queue_t *sleep_queue, tstate_t state, spinlock_t *lock);

/**
 * @brief Same as sched_sleep(), except for this one 'current'
 * is also rellocated to the fron of the tgroup queue.
 * 
 * @param sleep_queue sleep queue on which to place 'current'
 * @param state state in which we want the thread(current) to be.
 * @param lock  hand-over-lock passed to current upon waking up the thread.
 * @return int  0 on success otherwise an error code is returned.
 */
int sched_sleep_r(queue_t *sleep_queue, tstate_t state, spinlock_t *lock);

/**
 * @brief wake all threads on 'sleep-queue'
 *
 * @param sleep_queue
 * @return int the number of threads woken
 */
size_t sched_wakeall(queue_t *sleep_queue);

/*get a thread from a queue*/
thread_t *sched_next(void);

thread_t *sched_getembryo(void);
int sched_putembryo(thread_t *thread);

/*context switch back to the scheduler*/
extern void sched(void);

/*start running the scheduler*/
extern void schedule(void);