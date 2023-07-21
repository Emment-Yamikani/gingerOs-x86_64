#pragma once

#include <arch/x86_64/pml4.h>
#include <arch/x86_64/thread.h>
#include <lib/stddef.h>
#include <lib/stdint.h>
#include <lib/types.h>
#include <sync/spinlock.h>
#include <sys/system.h>
#include <arch/cpu.h>
#include <ds/queue.h>
#include <bits/errno.h>
#include <sync/cond.h>
#include <sys/_time.h>
#include <ginger/jiffies.h>

typedef enum {
    T_EMBRYO,
    T_READY,
    T_RUNNING,
    T_ISLEEP,
    T_STOPPED,
    T_TERMINATED,
    T_ZOMBIE,
} tstate_t;

__unused static const char *states[] = {
    [T_EMBRYO] = "EMBRYO",
    [T_READY] = "READY",
    [T_RUNNING] = "RUNNING",
    [T_ISLEEP] = "ISLEEP",
    [T_STOPPED] = "STOPPED",
    [T_TERMINATED] = "TERMINATED",
    [T_ZOMBIE] = "ZOMBIE",
    NULL,
};

typedef struct {
    time_t      ctime;      // Thread ceation time.
    time_t      cpu_time;   // CPU time in jiffies(n seconds = (jiffy * (HZ_TO_ns(SYS_HZ) / seconds_TO_ns(1))) ).
    atomic_t    timeslice;  // Quantum of CPU time for which this thread is allowed to run.
    time_t      total_time; // Total time this thread has run.
    time_t      last_sched; // Last time this thread was scheduled to run.

    cpu_t       *processor; // Processor for which this thread has affinity.
    atomic_t    affinity;   // Type of affinity (SOFT or HARD)
    atomic_t    priority;   // Thread scheduling Priority.

} sched_attr_t;

#define SCHED_ATTR_DEFAULT() (sched_attr_t){0}

/*soft affinity for the cpu*/
#define SCHED_SOFT_AFFINITY 0

/*hard affinity for the cpu*/
#define SCHED_HARD_AFFINITY 1

typedef struct {
    queue_t *queue;     // thread's sleep queue.
    queue_node_t *node; // thread's sleep node.
    spinlock_t *guard;  // non-null if sleep queue is associated with a quard lock.
} sleep_attr_t;

typedef struct {
    // file_t *table[NFILE];
    spinlock_t lock;
} file_table_t;

typedef struct
{
    tid_t           tgid;
    queue_t         *queue;
    atomic_t        nthreads;       // number of active thread(both running and sleeping)
    atomic_t        nrunning;       // No. of threads running.
    thread_t        *main_thread;   // the main thred of this group, i.e the very 1'st thread to run.
    thread_t        *last_thread;   // thread that is last to run in this thread group.
    thread_t        *waiter_thread; // thread that remains behind to clean up after other threads terminate.
    file_table_t    file;
    spinlock_t      lock;
} tgroup_t;

#define tgroup_assert(tg)           ({ assert(tg, "No thread group"); })
#define tgroup_lock(tg)             ({ tgroup_assert(tg); spin_lock(&(tg)->lock); })
#define tgroup_unlock(tg)           ({ tgroup_assert(tg); spin_unlock(&(tg)->lock); })
#define tgroup_locked(tg)           ({ tgroup_assert(tg); spin_locked(&(tg)->lock); })
#define tgroup_assert_locked(tg)    ({ tgroup_assert(tg); spin_assert_locked(&(tg)->lock); })


#define __tgroup_main     BS(0)   // set tgroup main thread.
#define __tgroup_waiter   BS(1)   // set tgroup waiter thread.
#define __tgroup_last     BS(2)   // set tgroup last thread.
#define __tgroup_all      (__tgroup_main | __tgroup_last | __tgroup_waiter)

int tgroup_set(tgroup_t *, thread_t *, int);
void tgroup_free(tgroup_t *);
int tgroup_new(tid_t, tgroup_t **);
void tgroup_wait_all(tgroup_t *tgrp);
int tgroup_kill_thread(tgroup_t *tgroup, tid_t tid);

typedef struct thread
{
    tid_t           t_tid;      // thread ID.
    tid_t           t_killer;   // thread that killed this thread.

    uintptr_t       t_entry;    // thread entry point.

    tstate_t        t_state;    // thread's execution state.

    uintptr_t       t_exit;     // thread exit code.
    atomic_t        t_flags;    // threads flags.
    uintptr_t       t_errno;    // thread's errno.

    void            *t_fpu_ctx;    
    pagemap_t       *t_map;     // thread's process virtual address space.

    sleep_attr_t    sleep_attr;  // struct describing sleep attributes for this thread.
    sched_attr_t    t_sched_attr;// struct describing scheduler attributes for this thread.

    tgroup_t        *t_group;    // thread group
    queue_t         *t_queues;   // queues on which this thread resides.

    cond_t          *t_wait;     // thread conditional wait variable.

    x86_64_thread_t t_arch;      // architecture thread struct.

    spinlock_t      t_lock;      // lock to synchronize access to this struct.
} __aligned(16) thread_t;

#define THREAD_USER                     BS(0)
#define THREAD_KILLED                   BS(1)
#define THREAD_SETPARK                  BS(2)
#define THREAD_SETWAKE                  BS(3)  
#define THREAD_HANDLING_SIG             BS(4)

#define thread_assert(t)                ({ assert_msg(t, "No thread pointer\n");})
#define thread_lock(t)                  ({ thread_assert(t); spin_lock(&((t)->t_lock)); })
#define thread_unlock(t)                ({ thread_assert(t); spin_unlock(&((t)->t_lock)); })
#define thread_locked(t)                ({ thread_assert(t); spin_locked(&((t)->t_lock)); })
#define thread_assert_locked(t)         ({ thread_assert(t); spin_assert_locked(&((t)->t_lock)); })

#define __thread_embryo(t)              ({ thread_assert_locked(t); ((t)->t_state == T_EMBRYO); })
#define __thread_ready(t)               ({ thread_assert_locked(t); ((t)->t_state == T_READY); })
#define __thread_isleep(t)              ({ thread_assert_locked(t); ((t)->t_state == T_ISLEEP); })
#define __thread_running(t)             ({ thread_assert_locked(t); ((t)->t_state == T_RUNNING); })
#define __thread_stopped(t)             ({ thread_assert_locked(t); ((t)->t_state == T_STOPPED); })
#define __thread_zombie(t)              ({ thread_assert_locked(t); ((t)->t_state == T_ZOMBIE); })
#define __thread_terminated(t)          ({ thread_assert_locked(t); ((t)->t_state == T_TERMINATED); })
#define __thread_testflags(t, flags)    ({ thread_assert_locked(t); atomic_read(&((t)->t_flags)) & (flags); })
#define __thread_setflags(t, flags)     ({ thread_assert_locked(t); atomic_fetch_or(&((t)->t_flags), (flags)); })
#define __thread_maskflags(t, flags)    ({ thread_assert_locked(t); atomic_fetch_and(&((t)->t_flags), ~(flags)); })
#define __thread_enter_state(t, state)  ({        \
    thread_assert_locked(t);                      \
    int err = 0;                                  \
    if ((state) < T_EMBRYO || (state) > T_ZOMBIE) \
        err = -EINVAL;                            \
    else                                          \
        (t)->t_state = state;                     \
    err;                                          \
})

#define __thread_killed(t) ({                            \
    int locked = thread_locked(t);                       \
    if (!locked)                                         \
        thread_lock(t);                                  \
    int killed = __thread_testflags((t), THREAD_KILLED); \
    if (!locked)                                         \
        thread_unlock(t);                                \
    killed;                                              \
})

#define __thread_ishandling_signal(t) ({                         \
    int locked = thread_locked(t);                               \
    if (!locked)                                                 \
        thread_lock(t);                                          \
    int handling = __thread_testflags((t), THREAD_HANDLING_SIG); \
    if (!locked)                                                 \
        thread_unlock(t);                                        \
    handling;                                                    \
})

#define current_assert()                ({ assert(current, "No current thread running"); })
#define current_lock()                  ({ thread_lock(current); })
#define current_unlock()                ({ thread_unlock(current); })
#define current_locked()                ({ thread_locked(current); })
#define current_assert_locked()         ({ thread_assert_locked(current); })

#define current_embryo()                ({ __thread_embryo(current); })
#define current_ready ()                ({ __thread_ready(current); })
#define current_isleep()                ({ __thread_isleep(current); })
#define current_running()               ({ __thread_running(current); })
#define current_stopped()               ({ __thread_stopped(current); })
#define current_zombie()                ({ __thread_zombie(current); })
#define current_terminated()            ({ __thread_terminated(current); })
#define current_setflags(flags)         ({ __thread_setflags(current, (flags)); })
#define current_testflags(flags)        ({ __thread_testflags(current, (flags)); })
#define current_maskflags(flags)        ({ __thread_maskflags(current, (flags)); })
#define current_enter_state(state)      ({ __thread_enter_state(current, (state)); })

#define BUILTIN_THREAD(name, entry, arg)                                                        \
    __section(".__builtin_thread_entry")    void *__CAT(__builtin_thread_entry_, name) = entry; \
    __section(".__builtin_thread_arg")      void *__CAT(__builtin_thread_arg_, name) = (void *)arg;

#define BUILTIN_THREAD_ANOUNCE(name)    ({ printk("\"%s\" thread [tid: %d] running...\n", name, thread_self()); })

#define KSTACKSZ    (512 * KiB)

int thread_new(thread_t **);
void thread_free(thread_t *);

thread_t *thread_dequeue(queue_t *queue);
int thread_remove_queue(thread_t *thread, queue_t *queue);
int thread_enqueue(queue_t *queue, thread_t *thread, queue_node_t **rnode);

int start_builtin_threads(int *nthreads, thread_t ***threads);

int kthread_create_join(void *(*entry)(void *), void *arg, void **ret);
int kthread_create(void *(*entry)(void *), void *arg, tid_t *__tid, thread_t **ref);

tid_t thread_self(void);
void thread_yield(void);
int thread_kill_all(void);
int thread_kill(tid_t tid);
int thread_cancel(tid_t tid);
int thread_wake(thread_t *thread);
int thread_kill_n(thread_t *thread);
void thread_exit(uintptr_t exit_code);
int thread_join(tid_t tid, void **retval);
int thread_get(tgroup_t *tgrp, tid_t tid, thread_t **tref);
int thread_wait(thread_t *thread, int reap, void **retval);
int thread_queue_get(queue_t *queue, tid_t tid, thread_t **pthread);